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

#include "utils/config.h"
#include "bricks/brick-int.h"

struct brick_config *diode_config_new(const char *name, uint32_t west_max,
				      uint32_t east_max, output_enum output)
{
	struct brick_config *config = g_new0(struct brick_config, 1);
	struct diode_config *diode_config = g_new0(struct diode_config, 1);

	brick_config__init(config);
	diode_config__init(diode_config);
	diode_config->output = output;
	config->diode = diode_config;
	return brick_config_init(config, name, west_max, east_max);
}

struct brick_config *brick_config_init(struct brick_config *config,
				       const char *name,
				       uint32_t west_max,
				       uint32_t east_max)
{
	config->name = g_strdup(name);
	config->west_max = west_max;
	config->east_max = east_max;
	return config;
}

struct brick_config *brick_config_new(const char *name, uint32_t west_max,
				      uint32_t east_max)
{
	struct brick_config *config = g_new0(struct brick_config, 1);

	brick_config__init(config);
	return brick_config_init(config, name, west_max, east_max);
}

void brick_config_free(struct brick_config *config)
{
	g_free(config->diode);
	g_free(config->name);
	g_free(config);
}

enum side output_to_side(output_enum output, struct switch_error **errp)
{
	switch (output) {
	case OUTPUT__TO_EAST:
		return EAST_SIDE;
	case OUTPUT__TO_WEST:
		return WEST_SIDE;
	default:
		*errp = error_new("Output contain an invalid value");
	}

	return MAX_SIDE;
}
