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

#include <packetgraph/brick.h>
#include <packetgraph/hub.h>

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
		struct brick_side *s = &brick->sides[i];

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

struct brick *hub_new(const char *name, uint32_t west_max,
		      uint32_t east_max,
		      struct switch_error **errp)
{
	struct brick_config *config = brick_config_new(name, west_max,
						       east_max);
	struct brick *ret = brick_new("hub", config, errp);

	brick_config_free(config);
	return ret;
}

static struct brick_ops hub_ops = {
	.name		= "hub",
	.state_size	= sizeof(struct hub_state),

	.init		= hub_init,

	.unlink		= brick_generic_unlink,
};

brick_register(hub, &hub_ops);
