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

#include "brick.h"
#include <packetgraph/nop.h>

struct pg_nop_state {
	struct pg_brick brick;
	int should_be_zero;
};

/* The fastpath data function of the nop_brick just forward the bursts */

static int nop_burst(struct pg_brick *brick, enum pg_side from,
		     uint16_t edge_index, struct rte_mbuf **pkts,
		     uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];

	if (s->edge.link == NULL)
		return 0;
	return  pg_brick_burst(s->edge.link, from,
			       s->edge.pair_index,
			       pkts, pkts_mask, errp);
}

static int nop_init(struct pg_brick *brick,
		    struct pg_brick_config *config,
		    struct pg_error **errp)
{
	struct pg_nop_state *state = pg_brick_get_state(brick,
							struct pg_nop_state);

	g_assert(!state->should_be_zero);

	/* initialize fast path */
	brick->burst = nop_burst;

	return 0;
}

struct pg_brick *pg_nop_new(const char *name,
			    struct pg_error **errp)
{
	struct pg_brick_config *config = pg_brick_config_new(name, 1, 1,
							     PG_DIPOLE);
	struct pg_brick *ret = pg_brick_new("nop", config, errp);

	pg_brick_config_free(config);

	return ret;
}

static struct pg_brick_ops nop_ops = {
	.name		= "nop",
	.state_size	= sizeof(struct pg_nop_state),

	.init		= nop_init,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(nop, &nop_ops);
