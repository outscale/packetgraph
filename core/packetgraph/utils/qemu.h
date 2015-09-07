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

#ifndef _UTILS_QEMU_H_
#define _UTILS_QEMU_H_

#include <packetgraph/utils/errors.h>

int pg_spawn_qemu(const char *socket_path_0, const char *socket_path_1,
		  const char *mac_0, const char *mac_1,
		  const char *bzimage_path,
		  const char *cpio_path,
		  const char *hugepages_path,
		  struct pg_error **errp);


#endif
