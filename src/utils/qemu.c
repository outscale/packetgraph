/* Copyright 2015 Nodalink EURL
 *
 * This file is part of Packetgraph.
 *
 * Packetgraph is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * Packetgraph is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Packetgraph.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/wait.h>
#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include "utils/qemu.h"
#include "utils/common.h"

int pg_util_cmdloop(const char *cmd, int timeout_s)
{
	struct timeval start, end;
	int status;

	gettimeofday(&start, 0);
	gettimeofday(&end, 0);
	while (end.tv_sec - start.tv_sec < timeout_s) {
		g_spawn_command_line_sync(cmd, NULL, NULL, &status, NULL);
		if (g_spawn_check_exit_status(status, NULL))
			return 0;
		gettimeofday(&end, 0);
	}
	return -1;
}

int pg_util_ssh(const char *host,
		int port,
		const char *key_path,
		const char *cmd, ...)
{
	pg_autofree char *c = NULL;
	pg_autofree char *ssh_cmd = NULL;
	va_list args;
	int status;

	va_start(args, cmd);
	c = g_strdup_vprintf(cmd, args);
	ssh_cmd = g_strdup_printf(
		PG_STRCAT("ssh %s -l root -q -i %s -p %u",
			  " -oConnectTimeout=1",
			  " -oStrictHostKeyChecking=no %s"),
		host, key_path, port, c);
	va_end(args);
	g_spawn_command_line_sync(ssh_cmd, NULL, NULL, &status, NULL);
	return g_spawn_check_exit_status(status, NULL) ? 0 : -1;
}

int pg_util_spawn_qemu(const char *socket_path_0,
		       const char *socket_path_1,
		       const char *mac_0,
		       const char *mac_1,
		       const char *vm_image_path,
		       const char *vm_key_path,
		       const char *hugepages_path,
		       struct pg_error **errp)
{
	int child_pid = 0;
	static uint16_t vm_id;
	char **argv = NULL;
	pg_autofree char *argv_qemu = NULL;
	const char *argv_sock_0 = "";
	const char *argv_sock_1 = "";
	pg_autofree char *argv_sock_0_t = NULL;
	pg_autofree char *argv_sock_1_t = NULL;
	pg_autofree char *ssh_cmd = NULL;
	GError *error = NULL;

	g_assert(g_file_test(socket_path_0, G_FILE_TEST_EXISTS));
	if (socket_path_1)
		g_assert(g_file_test(socket_path_1, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(vm_image_path, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(vm_key_path, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(hugepages_path, G_FILE_TEST_EXISTS));

	if (socket_path_0) {
		argv_sock_0_t = g_strdup_printf(
			PG_STRCAT(" -chardev socket,id=char0,path=%s",
				  " -netdev type=vhost-user,id=mynet0,",
				  "chardev=char0,vhostforce",
				  " -device virtio-net-pci,mac=%s",
				  ",netdev=mynet0"), socket_path_0, mac_0);
		argv_sock_0 = argv_sock_0_t;
	}

	if (socket_path_1) {
		argv_sock_1_t = g_strdup_printf(
			PG_STRCAT(" -chardev socket,id=char1,path=%s",
				  " -netdev type=vhost-user,id=mynet1,",
				  "chardev=char1,vhostforce",
				  " -device virtio-net-pci,mac=%s",
				  ",netdev=mynet1"), socket_path_1, mac_1);
		argv_sock_1 = argv_sock_1_t;
	}

	argv_qemu = g_strdup_printf(
		PG_STRCAT(
			"qemu-system-x86_64 -m 128M -enable-kvm",
			" -vnc :%u -display none -snapshot -object",
			" memory-backend-file,id=mem,size=128M,",
			"mem-path=%s,share=on -numa node,memdev=mem",
			" -mem-prealloc -drive file=%s",
			" -netdev user,id=net0,hostfwd=tcp::%u-:22",
			" -device e1000,netdev=net0%s%s",
			),
		vm_id, hugepages_path, vm_image_path,
		vm_id + 65000, argv_sock_0, argv_sock_1);


	argv = g_strsplit(argv_qemu, " ", 0);

	g_assert(g_spawn_async(NULL, argv, NULL,
			       (GSpawnFlags) G_SPAWN_SEARCH_PATH |
			       G_SPAWN_DO_NOT_REAP_CHILD,
			       (GSpawnChildSetupFunc) NULL,
			       NULL, &child_pid, &error));
	g_assert(!error);

	ssh_cmd = g_strdup_printf(
		PG_STRCAT("ssh root@localhost -q -i %s",
			  " -p %d -oConnectTimeout=1 ",
			  "-oStrictHostKeyChecking=no ",
			  "ls"), vm_key_path, vm_id + 65000);
	if (pg_util_cmdloop(ssh_cmd, 10 * 60) < 0)
		*errp = pg_error_new("qemu spawn failed");

	vm_id++;
	g_strfreev(argv);
	return child_pid;
}

void pg_util_stop_qemu(int qemu_pid, int signal)
{
	int exit_status;

	kill(qemu_pid, signal);
	waitpid(qemu_pid, &exit_status, 0);
	g_spawn_close_pid(qemu_pid);
}
