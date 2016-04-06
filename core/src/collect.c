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


#include <string.h>
#include <packetgraph/ccan/build_assert/build_assert.h>
#include <packetgraph/common.h>
#include <packetgraph/brick.h>
#include <packetgraph/packets.h>
#include <packetgraph/collect.h>

struct pg_collect_state {
	struct pg_brick brick;
	/* arrays of collected incoming packets */
	struct rte_mbuf *pkts[MAX_SIDE][PG_MAX_PKTS_BURST];
	uint64_t pkts_mask[MAX_SIDE];
};

static int collect_burst(struct pg_brick *brick, enum pg_side from,
			 uint16_t edge_index, struct rte_mbuf **pkts,
			 uint16_t nb, uint64_t pkts_mask,
			 struct pg_error **errp)
{
	struct pg_collect_state *state;

	state = pg_brick_get_state(brick, struct pg_collect_state);
	BUILD_ASSERT(PG_MAX_PKTS_BURST == 64);
	if (nb > PG_MAX_PKTS_BURST) {
		*errp = pg_error_new("Burst too big");
		return -1;
	}

	if (state->pkts_mask[from])
		pg_packets_free(state->pkts[from], state->pkts_mask[from]);

	state->pkts_mask[from] = pkts_mask;
	/* We made sure nb <= PG_MAX_PKTS_BURST */
	/* Flawfinder: ignore */
	memcpy(state->pkts[from], pkts, nb * sizeof(struct rte_mbuf *));
	pg_packets_incref(state->pkts[from], state->pkts_mask[from]);

	return 0;
}

static struct rte_mbuf **collect_burst_get(struct pg_brick *brick,
					   enum pg_side side,
					   uint64_t *pkts_mask)
{
	struct pg_collect_state *state;

	state = pg_brick_get_state(brick, struct pg_collect_state);
	if (!state->pkts_mask[side]) {
		*pkts_mask = 0;
		return NULL;
	}

	*pkts_mask = state->pkts_mask[side];
	return state->pkts[side];
}

static int collect_init(struct pg_brick *brick,
			struct pg_brick_config *config,
			struct pg_error **errp)
{
	brick->burst = collect_burst;

	return 1;
}

static int collect_reset(struct pg_brick *brick, struct pg_error **errp)
{
	enum pg_side i;
	struct pg_collect_state *state =
		pg_brick_get_state(brick, struct pg_collect_state);

	for (i = 0; i < MAX_SIDE; i++) {
		if (!state->pkts_mask[i])
			continue;

		pg_packets_free(state->pkts[i], state->pkts_mask[i]);
		pg_packets_forget(state->pkts[i], state->pkts_mask[i]);
		state->pkts_mask[i] = 0;
	}

	return 1;
}

struct pg_brick *pg_collect_new(const char *name,
				uint32_t west_max,
				uint32_t east_max,
				struct pg_error **errp)
{
	struct pg_brick_config *config;

	config = pg_brick_config_new(name, west_max, east_max, PG_MULTIPOLE);
	struct pg_brick *ret = pg_brick_new("collect", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static struct pg_brick_ops collect_ops = {
	.name		= "collect",
	.state_size	= sizeof(struct pg_collect_state),

	.init		= collect_init,

	.unlink		= pg_brick_generic_unlink,

	.reset		= collect_reset,
	.burst_get	= collect_burst_get,
};

pg_brick_register(collect, &collect_ops);
