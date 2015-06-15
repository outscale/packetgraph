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

#include <packetgraph/brick.h>
#include <packetgraph/nop.h>

struct nop_state {
	struct brick brick;
	int should_be_zero;
};

/* The fastpath data function of the nop_brick just forward the bursts */

static int nop_burst(struct brick *brick, enum side from, uint16_t edge_index,
		     struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		     struct switch_error **errp)
{
	struct brick_side *s = &brick->sides[flip_side(from)];

	return brick_side_forward(s, from, pkts, nb, pkts_mask, errp);
}

static int nop_init(struct brick *brick, struct brick_config *config,
		    struct switch_error **errp)
{
	struct nop_state *state = brick_get_state(brick, struct nop_state);

	g_assert(!state->should_be_zero);

	/* initialize fast path */
	brick->burst = nop_burst;

	return 1;
}

struct brick *nop_new(const char *name,
			    uint32_t west_max,
			    uint32_t east_max,
			    struct switch_error **errp)
{
	struct brick_config *config = brick_config_new(name, west_max,
						       east_max);
	struct brick *ret = brick_new("nop", config, errp);

	brick_config_free(config);

	return ret;
}

static struct brick_ops nop_ops = {
	.name		= "nop",
	.state_size	= sizeof(struct nop_state),

	.init		= nop_init,

	.unlink		= brick_generic_unlink,
};

brick_register(nop, &nop_ops);
