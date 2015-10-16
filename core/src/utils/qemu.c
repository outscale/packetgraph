/* Copyright 2015 Nodalink EURL
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
	uint a = 0;

	argv     = g_new(gchar *, 33);
#	define add_arg(str) { argv[a] = g_strdup(str); a++; }
#	define add_argp(str...) { argv[a] = g_strdup_printf(str); a++; }
	add_arg("qemu-system-x86_64");
	add_arg("-m");
	add_arg("124M");
	add_arg("-enable-kvm");
	add_arg("-kernel");
	add_arg(bzimage_path);
	add_arg("-initrd");
	add_arg(cpio_path);
	add_arg("-append");
	add_arg("console=ttyS0 rdinit=/sbin/init noapic");
	add_arg("-serial");
	add_arg("stdio");
	add_arg("-monitor");
	add_arg("/dev/null");
	add_arg("-nographic");

	/* Add interface if given. */
	if (socket_path_0) {
		add_arg("-chardev");
		add_argp("socket,id=char0,path=%s", socket_path_0);
		add_arg("-netdev");
		add_arg("type=vhost-user,id=mynet1,chardev=char0,vhostforce");
		add_arg("-device");
		add_argp("virtio-net-pci,mac=%s,netdev=mynet1", mac_0);
	}

	if (socket_path_1) {
		add_arg("-chardev");
		add_argp("socket,id=char1,path=%s", socket_path_1);
		add_arg("-netdev");
		add_arg("type=vhost-user,id=mynet2,chardev=char1,vhostforce");
		add_arg("-device");
		add_argp("virtio-net-pci,mac=%s,netdev=mynet2", mac_1);
	}
	add_arg("-object");
	add_argp("memory-backend-file,id=mem,size=124M,mem-path=%s,share=on",
		 hugepages_path);
	add_arg("-numa");
	add_arg("node,memdev=mem");
	add_arg("-mem-prealloc");

#	undef add_arg
	argv[a] = NULL;

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
