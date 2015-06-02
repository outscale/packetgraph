/* Copyright 2014 Outscale SAS
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

#include "bricks/brick.h"

struct diode_state {
	struct brick brick;
	enum side output;
};

static int diode_burst(struct brick *brick, enum side from, uint16_t edge_index,
		       struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		       struct switch_error **errp)
{
	struct diode_state *state = brick_get_state(brick, struct diode_state);
	struct brick_side *s = &brick->sides[flip_side(from)];

	if (state->output == from)
		return 1;

	return brick_side_forward(s, from, pkts, nb, pkts_mask, errp);
}

static int diode_init(struct brick *brick,
		      struct brick_config *config, struct switch_error **errp)
{
	struct diode_state *state = brick_get_state(brick, struct diode_state);
	struct diode_config *diode_config;

	if (!config->diode) {
		*errp = error_new("config->diode is NULL");
		return 0;
	}

	diode_config = config->diode;

	brick->burst = diode_burst;

	state->output = diode_config->output;
	if (error_is_set(errp))
		return 0;

	return 1;
}

struct brick *diode_new(const char *name,
			      uint32_t west_max,
			      uint32_t east_max,
			      enum side output,
			      struct switch_error **errp)
{
	struct brick_config *config = diode_config_new(name, west_max,
						       east_max, output);
	struct brick *ret = brick_new("diode", config, errp);

	brick_config_free(config);
	return ret;
}

static struct brick_ops diode_ops = {
	.name		= "diode",
	.state_size	= sizeof(struct diode_state),

	.init		= diode_init,

	.unlink		= brick_generic_unlink,
};

brick_register(struct diode_state, &diode_ops);
