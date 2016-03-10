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

struct pg_hub_state {
	struct pg_brick brick;
};

static int hub_burst(struct pg_brick *brick, enum pg_side from,
		     uint16_t edge_index, struct rte_mbuf **pkts,
		     uint16_t nb, uint64_t pkts_mask,
		     struct pg_error **errp)
{
	int ret;
	enum pg_side i;
	uint16_t j;

	for (i = 0; i < MAX_SIDE; i++) {
		struct pg_brick_side *s = &brick->sides[i];

		for (j = 0; j < s->max; j++) {
			if (!s->edges[j].link)
				continue;
			if (from == i && j == edge_index)
				continue;
			ret = pg_brick_burst(s->edges[j].link, pg_flip_side(i),
					     s->edges[j].pair_index,
					     pkts, nb, pkts_mask, errp);
			if (unlikely(!ret))
				return -1;
		}
	}
	return 0;
}

static int hub_init(struct pg_brick *brick,
		    struct pg_brick_config *config,
		    struct pg_error **errp)
{
	/* initialize fast path */
	brick->burst = hub_burst;

	return 1;
}

struct pg_brick *pg_hub_new(const char *name, uint32_t west_max,
			    uint32_t east_max,
			    struct pg_error **errp)
{
	struct pg_brick_config *config = pg_brick_config_new(name, west_max,
							     east_max,
							     PG_MULTIPOLE);
	struct pg_brick *ret = pg_brick_new("hub", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static struct pg_brick_ops hub_ops = {
	.name		= "hub",
	.state_size	= sizeof(struct pg_hub_state),

	.init		= hub_init,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(hub, &hub_ops);
