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

static int diode_burst(struct brick *brick, enum side side, uint16_t edge_index,
		       struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		       struct switch_error **errp)
{
	struct diode_state *state = brick_get_state(brick, struct diode_state);
	struct brick_side *s = &brick->sides[flip_side(side)];

	if (state->output == side)
		return 1;

	return brick_side_forward(s, side, pkts, nb, pkts_mask, errp);
}

static int diode_init(struct brick *brick,
		      BrickConfig *config, struct switch_error **errp)
{
	struct diode_state *state = brick_get_state(brick, struct diode_state);
	struct diode_config *diode_config;

	if (!config->diode) {
		*errp = error_new("config->diode is NULL");
		return 0;
	}

	diode_config = config->diode;

	brick->burst = diode_burst;

	state->output = output_to_side(diode_config->output, errp);
	if (error_is_set(errp))
		return 0;

	return 1;
}

static struct brick_ops diode_ops = {
	.name		= "diode",
	.state_size	= sizeof(struct diode_state),

	.init		= diode_init,

	.west_link	= brick_generic_west_link,
	.east_link	= brick_generic_east_link,
	.unlink		= brick_generic_unlink,
};

brick_register(struct diode_state, &diode_ops);
