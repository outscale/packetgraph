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

#include <glib.h>

#include <packetgraph/utils/config.h>
#include <packetgraph/brick-int.h>

struct pg_brick_config *pg_brick_config_init(struct pg_brick_config *config,
					  const char *name,
					  uint32_t west_max,
					  uint32_t east_max)
{
	config->name = g_strdup(name);
	config->west_max = west_max;
	config->east_max = east_max;
	config->brick_config_free = NULL;
	return config;
}

struct pg_brick_config *pg_brick_config_new(const char *name,
					    uint32_t west_max,
					    uint32_t east_max)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);

	return pg_brick_config_init(config, name, west_max, east_max);
}

void pg_brick_config_free(struct pg_brick_config *config)
{
	if (config->brick_config_free)
		config->brick_config_free(config->brick_config);
	g_free(config->brick_config);
	g_free(config->name);
	g_free(config);
}
