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

#ifndef _UTILS_CONFIG_H_
#define _UTILS_CONFIG_H_

#include <packetgraph/common.h>
#include <packetgraph/utils/errors.h>

struct vtep_config {
	enum side output;
	int32_t ip;
	struct ether_addr mac;
};

struct brick_config {
	/* The unique name of the brick brick in the graph */
	char *name;
	/* The maximum number of west edges */
	uint32_t west_max;
	/* The maximum number of east edges */
	uint32_t east_max;

	/* Function to call to free this brick config */
	void (*brick_config_free)(void *brick_config);
	void *brick_config;
};

struct brick_config *brick_config_init(struct brick_config *config,
				       const char *name,
				       uint32_t west_max,
				       uint32_t east_max);

struct brick_config *brick_config_new(const char *name, uint32_t west_max,
				      uint32_t east_max);

void brick_config_free(struct brick_config *config);

#endif
