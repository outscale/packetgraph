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
#include <packetgraph/brick.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/packets.h>
#include <packetgraph/common.h>
#include <packetgraph/packetsgen.h>

#ifndef PACKETSGEN_POLL_NB_PKTS
#define PACKETSGEN_POLL_NB_PKTS 3
#endif

struct pg_packetsgen_config {
	enum pg_side output;
	struct rte_mbuf **packets;
	uint16_t packets_nb;
};

struct pg_packetsgen_state {
	struct pg_brick brick;
	enum pg_side output;
	struct rte_mbuf **packets;
	uint16_t packets_nb;
};

/* The fastpath data function of the packetsgen_brick just forward the bursts */

static int packetsgen_burst(struct pg_brick *brick, enum pg_side side,
			    uint16_t edge_index,
			    struct rte_mbuf **pkts, uint16_t nb,
			    uint64_t pkts_mask,
			    struct pg_error **errp)
{
	struct pg_brick_side *s = &brick->sides[pg_flip_side(side)];

	return pg_brick_side_forward(s, side, pkts, nb, pkts_mask, errp);
}

static int packetsgen_poll(struct pg_brick *brick,
			   uint16_t *pkts_cnt,
			   struct pg_error **errp)
{
	struct pg_packetsgen_state *state;
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf **pkts;
	struct pg_brick_side *s;
	uint64_t pkts_mask;
	int ret;
	uint16_t i;

	state = pg_brick_get_state(brick, struct pg_packetsgen_state);
	s = &brick->sides[state->output];


	pkts = g_new0(struct rte_mbuf*, state->packets_nb);
	for (i = 0; i < state->packets_nb; i++) {
		pkts[i] = rte_pktmbuf_clone(state->packets[i], mp);
		pkts[i]->udata64 = i;
	}

	pkts_mask = pg_mask_firsts(state->packets_nb);
	*pkts_cnt = state->packets_nb;
	ret = pg_brick_side_forward(s, pg_flip_side(state->output),
				    pkts, state->packets_nb,
				    pkts_mask, errp);
	pg_packets_free(pkts, pkts_mask);
	g_free(pkts);
	return ret;
}

#undef PACKETSGEN_POLL_NB_PKTS

static int packetsgen_init(struct pg_brick *brick,
			   struct pg_brick_config *config,
			   struct pg_error **errp)
{
	struct pg_packetsgen_state *state;
	struct pg_packetsgen_config *packetsgen_config;

	state = pg_brick_get_state(brick, struct pg_packetsgen_state);

	if (!config->brick_config) {
		*errp = pg_error_new("config->brick_config is NULL");
		return 0;
	}

	if (state->packets == NULL) {
		*errp = pg_error_new("packets argument is NULL");
		return 0;
	}

	if (state->packets_nb == 0) {
		*errp = pg_error_new("packet number must be positive");
		return 0;
	}

	packetsgen_config = (struct pg_packetsgen_config *)
		config->brick_config;
	state->output = packetsgen_config->output;
	state->packets = packetsgen_config->packets;
	state->packets_nb = packetsgen_config->packets_nb;

	if (pg_error_is_set(errp))
		return 0;

	/* Initialize fast path */
	brick->burst = packetsgen_burst;
	brick->poll = packetsgen_poll;

	return 1;
}

static struct pg_brick_config *packetsgen_config_new(const char *name,
						     uint32_t west_max,
						     uint32_t east_max,
						     struct rte_mbuf **packets,
						     uint16_t packets_nb,
						     enum pg_side output)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_packetsgen_config *packetsgen_config =
		g_new0(struct pg_packetsgen_config, 1);

	packetsgen_config->output = output;
	packetsgen_config->packets = packets;
	packetsgen_config->packets_nb = packets_nb;
	config->brick_config = (void *) packetsgen_config;
	return pg_brick_config_init(config, name, west_max, east_max);
}


struct pg_brick *pg_packetsgen_new(const char *name,
				   uint32_t west_max,
				   uint32_t east_max,
				   enum pg_side output,
				   struct rte_mbuf **packets,
				   uint16_t packets_nb,
				   struct pg_error **errp)
{
	struct pg_brick_config *config;

	config = packetsgen_config_new(name,
				       west_max,
				       east_max,
				       packets,
				       packets_nb,
				       output);
	struct pg_brick *ret = pg_brick_new("packetsgen", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static struct pg_brick_ops packetsgen_ops = {
	.name		= "packetsgen",
	.state_size	= sizeof(struct pg_packetsgen_state),

	.init		= packetsgen_init,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(packetsgen, &packetsgen_ops);
