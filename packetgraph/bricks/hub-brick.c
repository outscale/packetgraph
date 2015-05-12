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

struct hub_state {
	struct brick brick;
};

static int hub_burst(struct brick *brick, enum side from, uint16_t edge_index,
		     struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		     struct switch_error **errp)
{
	int ret;
	enum side i;
	uint16_t j;

	for (i = 0; i < MAX_SIDE; i++) {
		struct brick_side *s = &brick->sides[flip_side(i)];

		for (j = 0; j < s->max; j++) {
			if (!s->edges[j].link)
				continue;
			if (from == i && j == edge_index)
				continue;
			ret = brick_burst(s->edges[j].link, flip_side(i),
					  s->edges[j].pair_index,
					  pkts, nb, pkts_mask, errp);
			if (unlikely(!ret))
				return 0;
		}
	}
	return 1;
}

static int hub_init(struct brick *brick,
		    struct brick_config *config,
		    struct switch_error **errp)
{
	/* initialize fast path */
	brick->burst = hub_burst;

	return 1;
}

static struct brick_ops hub_ops = {
	.name		= "hub",
	.state_size	= sizeof(struct hub_state),

	.init		= hub_init,

	.west_link	= brick_generic_west_link,
	.east_link	= brick_generic_east_link,
	.unlink		= brick_generic_unlink,
};

brick_register(struct hub_state, &hub_ops);
