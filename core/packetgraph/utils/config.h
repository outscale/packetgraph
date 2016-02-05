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

#ifndef _PG_CORE_UTILS_CONFIG_H
#define _PG_CORE_UTILS_CONFIG_H

#include <packetgraph/common.h>
#include <packetgraph/utils/errors.h>

enum pg_brick_type {
	PG_MULTIPOLE,
	PG_MONOPOLE,
	PG_DIPOLE
};

struct pg_brick_config {
	/* The unique name of the brick brick in the graph */
	char *name;
	/* The maximum number of west edges */
	uint32_t west_max;
	/* The maximum number of east edges */
	uint32_t east_max;

	enum pg_brick_type  type;
	/* Function to call to free this brick config */
	void (*brick_config_free)(void *brick_config);
	void *brick_config;
};

static inline const char *pg_brick_type_to_string(enum pg_brick_type type)
{
	static const char * const type_str[] = {"PG_MULTIPOLE",
						"PG_MONOPOLE",
						"PG_DIPOLE"};

	return type_str[type];
}

struct pg_brick_config *pg_brick_config_init(struct pg_brick_config *config,
					     const char *name,
					     uint32_t west_max,
					     uint32_t east_max,
					     enum pg_brick_type type);

struct pg_brick_config *pg_brick_config_new(const char *name,
					    uint32_t west_max,
					    uint32_t east_max,
					    enum pg_brick_type type);

void pg_brick_config_free(struct pg_brick_config *config);

#endif /* _PG_CORE_UTILS_CONFIG_H */
