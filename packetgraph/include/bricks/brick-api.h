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

enum side flip_side(enum side side);

/* testing */
int64_t brick_refcount(struct brick *brick);

/* relationship between bricks */
int brick_west_link(struct brick *target,
		    struct brick *brick, struct switch_error **errp);
int brick_east_link(struct brick *target,
		    struct brick *brick, struct switch_error **errp);
void brick_unlink(struct brick *brick, struct switch_error **errp);

/* polling */
int brick_poll(struct brick *brick,
		      uint16_t *count, struct switch_error **errp);

/* pkts count */
int64_t brick_pkts_count_get(struct brick *brick, enum side side);

/* constructors */
struct brick *nop_new(const char *name,
		      uint32_t west_max,
		      uint32_t east_max,
		      struct switch_error **errp);


struct brick *packetsgen_new(const char *name,
			uint32_t west_max,
			uint32_t east_max,
			enum side output,
			struct switch_error **errp);

struct brick *diode_new(const char *name,
			uint32_t west_max,
			uint32_t east_max,
			enum side output,
			struct switch_error **errp);

struct brick *vhost_new(const char *name, uint32_t west_max,
			uint32_t east_max, enum side output,
			struct switch_error **errp);

struct brick *switch_new(const char *name, uint32_t west_max,
			uint32_t east_max,
			struct switch_error **errp);


/* destructor */
void brick_destroy(struct brick *brick);

#endif
