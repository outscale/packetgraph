/* Copyright 2014 Nodalink EURL
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

#ifndef _BRICKS_BRICK_API_H_
#define _BRICKS_BRICK_API_H_

#include "utils/errors.h"
#include "utils/config.h"

struct brick;

inline enum side flip_side(enum side side);

/* testing */
int64_t brick_refcount(struct brick *brick);

/* relationship between bricks */
int brick_west_link(struct brick *target,
		    struct brick *brick, struct switch_error **errp);
int brick_east_link(struct brick *target,
		    struct brick *brick, struct switch_error **errp);
void brick_unlink(struct brick *brick, struct switch_error **errp);

/* polling */
inline int brick_poll(struct brick *brick,
		      uint16_t *count, struct switch_error **errp);

/* pkts count */
inline int64_t brick_pkts_count_get(struct brick *brick, enum side side);

#endif
