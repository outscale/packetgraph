/* Copyright 2014 Outscale SAS
 *
 * This file is part of Packetgraph.
 *
 * Packetgraph is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * Packetgraph is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Packetgraph.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <packetgraph/packetgraph.h>
#include "brick-int.h"

struct pg_hub_state {
	struct pg_brick brick;
};

static int hub_burst_one_side(struct pg_brick *brick, enum pg_side from,
			      uint16_t edge_index, struct rte_mbuf **pkts,
			      uint64_t pkts_mask,
			      struct pg_error **errp)

{	int ret;
	enum pg_side i = PG_WEST_SIDE;
	enum pg_side flip_i = PG_WEST_SIDE;
	uint16_t j;
	struct pg_brick_side *s;
	struct pg_brick_edge *edges;

	if (from == i)
		flip_i = pg_flip_side(i);
	else
		i = pg_flip_side(i);

	flip_i = pg_flip_side(flip_i);
	i = pg_flip_side(i);
	s = &brick->sides[i];
	edges = s->edges;

	for (j = 0; j < s->max; ++j) {
		if (!edges[j].link)
			continue;
		ret = pg_brick_burst(edges[j].link,
				     flip_i,
				     edges[j].pair_index,
				     pkts, pkts_mask, errp);
		if (unlikely(ret < 0))
			return -1;
	}
	return 0;

}


static int hub_burst(struct pg_brick *brick, enum pg_side from,
		     uint16_t edge_index, struct rte_mbuf **pkts,
		     uint64_t pkts_mask,
		     struct pg_error **errp)
{
	int ret;
	enum pg_side i = PG_WEST_SIDE;
	enum pg_side flip_i = PG_WEST_SIDE;
	uint16_t j;
	struct pg_brick_side *s;
	struct pg_brick_edge *edges;

	if (from == i)
		flip_i = pg_flip_side(i);
	else
		i = pg_flip_side(i);

	s = &brick->sides[i];
	edges = s->edges;

	for (j = 0; j < s->max; ++j) {
		if (!edges[j].link || j == edge_index)
			continue;
		ret = pg_brick_burst(edges[j].link,
				     flip_i,
				     edges[j].pair_index,
				     pkts, pkts_mask, errp);
		if (unlikely(ret < 0))
			return -1;
	}

	flip_i = pg_flip_side(flip_i);
	i = pg_flip_side(i);
	s = &brick->sides[i];
	edges = s->edges;

	for (j = 0; j < s->max; ++j) {
		if (!edges[j].link)
			continue;
		ret = pg_brick_burst(edges[j].link,
				     flip_i,
				     edges[j].pair_index,
				     pkts, pkts_mask, errp);
		if (unlikely(ret < 0))
			return -1;
	}
	return 0;
}

void pg_hub_set_no_backward(struct pg_brick *brick, int val)
{
	if (val)
		brick->burst = hub_burst_one_side;
	else
		brick->burst = hub_burst;
}

static int hub_init(struct pg_brick *brick,
		    struct pg_brick_config *config,
		    struct pg_error **errp)
{
	/* initialize fast path */
	brick->burst = hub_burst;

	return 0;
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
