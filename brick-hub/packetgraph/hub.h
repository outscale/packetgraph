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

#ifndef _BRICKS_BRICK_HUB_H_
#define _BRICKS_BRICK_HUB_H_

#include <packetgraph/utils/errors.h>

/**
 * Create a new hub brick
 *
 * @param	name name of the brick
 * @param	west_max maximum of links you can connect on the west side
 * @param	east_max maximum of links you can connect on the east side
  * @param	errp is set in case of an error
 * @return	a pointer to a brick structure, on success, 0 on error
 */
struct brick *hub_new(const char *name, uint32_t west_max,
			uint32_t east_max,
			struct switch_error **errp);

#endif  /* _BRICKS_BRICK_HUB_H_ */
