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

#ifndef _BRICKS_BRICK_VHOST_H_
#define _BRICKS_BRICK_VHOST_H_

#include <packetgraph/brick.h>

/**
 * Create a new nic brick
 *
 * @name:	name of the brick
 * @west_max:	maximum of links you can connect on the west side
 * @east_max:	maximum of links you can connect on the east side
 * @output:	The side of the output (so the side of the "VM")
 * @errp:	set in case of an error
 * @return:	a pointer to a brick structure, on success, 0 on error
 */
struct brick *vhost_new(const char *name, uint32_t west_max,
			uint32_t east_max, enum side output,
			struct switch_error **errp);

/**
 * Initialize vhost-user at program startup
 *
 * Does not take care of cleaning up everything on failure since
 * the program will abort in this case.
 *
 * @param: the base directory where the vhost-user socket will be
 * @param: an error pointer
 * @return: 1 on success, 0 on error
 */
int vhost_start(const char *base_dir, struct switch_error **errp);

void vhost_stop(void);

#endif  /* _BRICKS_BRICK_VHOST_H_ */
