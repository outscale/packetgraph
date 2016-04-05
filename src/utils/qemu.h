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

#ifndef _PG_UTILS_QEMU_H
#define _PG_UTILS_QEMU_H

#include <packetgraph/errors.h>

int pg_util_cmdloop(const char *cmd, int timeout_s);

int pg_util_ssh(const char *host,
		int port,
		const char *key_path,
		const char *cmd);

int pg_util_spawn_qemu(const char *socket_path_0,
		       const char *socket_path_1,
		       const char *mac_0,
		       const char *mac_1,
		       const char *vm_image_path,
		       const char *vm_ssh_key_path,
		       const char *hugepages_path,
		       struct pg_error **errp);

void pg_util_stop_qemu(int qemu_pid);

#endif /* _PG_UTILS_MEMPOOL_H */
