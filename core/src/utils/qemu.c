/* Copyright 2015 Outscale SAS
 *
 * This file is part of Butterfly.
 *
 * Butterfly is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * Butterfly is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Butterfly.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <packetgraph/utils/qemu.h>

static int is_ready(char *str)
{
	if (!str)
		return 0;
	return !strncmp(str, "Ready", strlen("Ready"));
}

int pg_spawn_qemu(const char *socket_path_0,
		  const char *socket_path_1,
		  const char *mac_0,
		  const char *mac_1,
		  const char *bzimage_path,
		  const char *cpio_path,
		  const char *hugepages_path,
		  struct pg_error **errp)
{
	GIOChannel *stdout_gio;
	GError *error = NULL;
	gchar *str_stdout;
	/* gchar *str_stderr; */
	int  child_pid;
	gint stdout_fd;
	gchar **argv;
	int readiness = 1;
	struct timeval start, end;
	
	argv     = g_new(gchar *, 33);
	argv[0]  = g_strdup("qemu-system-x86_64");
	argv[1]  = g_strdup("-m");
	argv[2]  = g_strdup("124M");
	argv[3]  = g_strdup("-enable-kvm");
	argv[4]  = g_strdup("-kernel");
	argv[5]  = g_strdup(bzimage_path);
	argv[6]  = g_strdup("-initrd");
	argv[7]  = g_strdup(cpio_path);
	argv[8]  = g_strdup("-append");
	argv[9]  = g_strdup("console=ttyS0 rdinit=/sbin/init noapic");
	argv[10] = g_strdup("-serial");
	argv[11] = g_strdup("stdio");
	argv[12] = g_strdup("-monitor");
	argv[13] = g_strdup("/dev/null");
	argv[14] = g_strdup("-nographic");

	argv[15] = g_strdup("-chardev");
	argv[16] = g_strdup_printf("socket,id=char0,path=%s", socket_path_0);
	argv[17] = g_strdup("-netdev");
	argv[18] =
		g_strdup("type=vhost-user,id=mynet1,chardev=char0,vhostforce");
	argv[19] = g_strdup("-device");
	argv[20] = g_strdup_printf("virtio-net-pci,mac=%s,netdev=mynet1",
				   mac_0);
	argv[21] = g_strdup("-chardev");
	argv[22] = g_strdup_printf("socket,id=char1,path=%s", socket_path_1);
	argv[23] = g_strdup("-netdev");
	argv[24] =
		g_strdup("type=vhost-user,id=mynet2,chardev=char1,vhostforce");
	argv[25] = g_strdup("-device");
	argv[26] = g_strdup_printf("virtio-net-pci,mac=%s,netdev=mynet2",
				   mac_1);
	argv[27] = g_strdup("-object");
	argv[28] =
		g_strdup_printf("memory-backend-file,id=mem,size=124M,mem-path=%s,share=on",
				hugepages_path);
	argv[29] = g_strdup("-numa");
	argv[30] = g_strdup("node,memdev=mem");
	argv[31] = g_strdup("-mem-prealloc");

	argv[32] = NULL;

	if (!g_spawn_async_with_pipes(NULL,
				      argv,
				      NULL,
				      (GSpawnFlags) G_SPAWN_SEARCH_PATH |
				      G_SPAWN_DO_NOT_REAP_CHILD,
				      (GSpawnChildSetupFunc) NULL,
				      NULL,
				      &child_pid,
				      NULL,
				      &stdout_fd,
				      NULL,
				      &error))
		g_assert(0);
	g_assert(!error);
	g_strfreev(argv);

	stdout_gio = g_io_channel_unix_new(stdout_fd);
	
	g_io_channel_read_line(stdout_gio,
			       &str_stdout,
			       NULL,
			       NULL,
			       &error);


	g_assert(!error);
	gettimeofday(&start, 0);
	while (!is_ready(str_stdout)) {
		g_free(str_stdout);
		g_io_channel_read_line(stdout_gio,
				       &str_stdout,
				       NULL,
				       NULL,
				       &error);

		g_assert(!error);
		gettimeofday(&end, 0);
		if ((end.tv_sec - start.tv_sec) > 30) {
			readiness = 0;
			break;
		}
	}

	if (!readiness)
		*errp = pg_error_new("qemu spawming timeout");

	g_free(str_stdout);

	g_io_channel_shutdown(stdout_gio, TRUE, &error);
	g_io_channel_unref(stdout_gio);

	return child_pid;
}
