/* Copyright 2015 Outscale SAS
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

/*
 * This brick generate somes packets configure with PACKETSGEN_POLL_NB_PKTS
 */

#include <string.h>
#include "bricks/brick.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "packets/packets.h"
#include "common.h"

#ifndef PACKETSGEN_POLL_NB_PKTS
#define PACKETSGEN_POLL_NB_PKTS 3
#endif
struct packetsgen_state {
	struct brick brick;
	enum side output;
};

/* The fastpath data function of the packetsgen_brick just forward the bursts */

static int packetsgen_burst(struct brick *brick, enum side side,
			    uint16_t edge_index,
			    struct rte_mbuf **pkts, uint16_t nb,
			    uint64_t pkts_mask,
			    struct switch_error **errp)
{
	struct brick_side *s = &brick->sides[flip_side(side)];

	return brick_side_forward(s, side, pkts, nb, pkts_mask, errp);
}

static int packetsgen_poll(struct brick *brick, uint16_t *pkts_cnt,
		      struct switch_error **errp)
{
	struct packetsgen_state *state;
	struct rte_mempool *mp = get_mempool();
	struct rte_mbuf *pkts[PACKETSGEN_POLL_NB_PKTS];
	struct brick_side *s;
	uint64_t pkts_mask;
	int ret;
	uint16_t i;

	state = brick_get_state(brick, struct packetsgen_state);
	s = &brick->sides[state->output];
	memset(pkts, 0, PACKETSGEN_POLL_NB_PKTS * sizeof(struct rte_mbuf *));
	for (i = 0; i < PACKETSGEN_POLL_NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mp);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
	}
	pkts_mask = mask_firsts(PACKETSGEN_POLL_NB_PKTS);
	*pkts_cnt = PACKETSGEN_POLL_NB_PKTS;
	ret = brick_side_forward(s, flip_side(state->output),
				 pkts, PACKETSGEN_POLL_NB_PKTS,
				 pkts_mask, errp);
	packets_free(pkts, pkts_mask);
	return ret;
}

#undef PACKETSGEN_POLL_NB_PKTS

static int packetsgen_init(struct brick *brick, struct brick_config *config,
		      struct switch_error **errp)
{
	struct packetsgen_state *state;

	state = brick_get_state(brick, struct packetsgen_state);
	/* We use a diode config because we need the output parameter */
	if (!config->diode) {
		*errp = error_new("config->diode is NULL");
		return 0;
	}

	state->output = config->diode->output;
	if (error_is_set(errp))
		return 0;

	/* Initialize fast path */
	brick->burst = packetsgen_burst;
	brick->poll = packetsgen_poll;

	return 1;
}

static struct brick_ops packetsgen_ops = {
	.name		= "packetsgen",
	.state_size	= sizeof(struct packetsgen_state),

	.init		= packetsgen_init,

	.west_link	= brick_generic_west_link,
	.east_link	= brick_generic_east_link,
	.unlink		= brick_generic_unlink,
};

brick_register(struct packetsgen_state, &packetsgen_ops);
