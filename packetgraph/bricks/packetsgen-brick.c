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
	struct rte_mbuf **packets;
	uint16_t packets_nb;
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
	struct rte_mbuf **pkts;
	struct brick_side *s;
	uint64_t pkts_mask;
	int ret;
	uint16_t i;

	state = brick_get_state(brick, struct packetsgen_state);
	s = &brick->sides[state->output];

	if (state->packets == NULL) {
		pkts = g_new0(struct rte_mbuf*, PACKETSGEN_POLL_NB_PKTS);
		memset(pkts, 0, PACKETSGEN_POLL_NB_PKTS *
		       sizeof(struct rte_mbuf *));
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
		g_free(pkts);
	} else {
		pkts = g_new0(struct rte_mbuf*, state->packets_nb);
		for (i = 0; i < state->packets_nb; i++)
			pkts[i] = rte_pktmbuf_clone(state->packets[i], mp);
		pkts_mask = mask_firsts(state->packets_nb);
		*pkts_cnt = state->packets_nb;
		ret = brick_side_forward(s, flip_side(state->output),
					 pkts, state->packets_nb,
					 pkts_mask, errp);
		g_free(pkts);
	}
	return ret;
}

#undef PACKETSGEN_POLL_NB_PKTS

static int packetsgen_init(struct brick *brick, struct brick_config *config,
			   struct switch_error **errp)
{
	struct packetsgen_state *state;

	state = brick_get_state(brick, struct packetsgen_state);
	if (!config->packetsgen) {
		*errp = error_new("config->packetsgen is NULL");
		return 0;
	}

	state->output = config->packetsgen->output;
	state->packets = config->packetsgen->packets;
	state->packets_nb = config->packetsgen->packets_nb;
	if (error_is_set(errp))
		return 0;

	/* Initialize fast path */
	brick->burst = packetsgen_burst;
	brick->poll = packetsgen_poll;

	return 1;
}

struct brick *packetsgen_new(const char *name,
			     uint32_t west_max,
			     uint32_t east_max,
			     enum side output,
			     struct rte_mbuf **packets,
			     uint16_t packets_nb,
			     struct switch_error **errp)
{
	struct brick_config *config = packetsgen_config_new(name, west_max,
							    east_max, output,
							    packets,
							    packets_nb);
	struct brick *ret = brick_new("packetsgen", config, errp);

	brick_config_free(config);
	return ret;
}

static struct brick_ops packetsgen_ops = {
	.name		= "packetsgen",
	.state_size	= sizeof(struct packetsgen_state),

	.init		= packetsgen_init,

	.unlink		= brick_generic_unlink,
};

brick_register(struct packetsgen_state, &packetsgen_ops);
