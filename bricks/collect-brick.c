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
#include <ccan/build_assert/build_assert.h>

#include "switch-config.h"
#include "bricks/brick.h"
#include "packets/packets.h"

struct collect_state {
	struct brick brick;
	/* arrays of collected incoming packets */
	struct rte_mbuf *pkts[MAX_SIDE][MAX_PKTS_BURST];
	uint64_t pkts_mask[MAX_SIDE];
};

static int collect_burst(struct brick *brick, enum side side,
			 uint16_t edge_index, struct rte_mbuf **pkts,
			 uint16_t nb, uint64_t pkts_mask,
			 struct switch_error **errp)
{
	struct collect_state *state =
		brick_get_state(brick, struct collect_state);

	BUILD_ASSERT(MAX_PKTS_BURST == 64);
	if (nb > MAX_PKTS_BURST) {
		*errp = error_new("Burst too big");
		return 0;
	}

	if (state->pkts_mask[side])
		packets_free(state->pkts[side], state->pkts_mask[side]);

	state->pkts_mask[side] = pkts_mask;
	/* We made sure nb <= MAX_PKTS_BURST */
	/* Flawfinder: ignore */
	memcpy(state->pkts[side], pkts, nb * sizeof(struct rte_mbuf *));
	packets_incref(state->pkts[side], state->pkts_mask[side]);

	return 1;
}

static struct rte_mbuf **collect_burst_get(struct brick *brick, enum side side,
					   uint64_t *pkts_mask)
{
	struct collect_state *state =
		brick_get_state(brick, struct collect_state);

	if (!state->pkts_mask[side]) {
		*pkts_mask = 0;
		return NULL;
	}

	*pkts_mask = state->pkts_mask[side];
	return state->pkts[side];
}

static int collect_init(struct brick *brick,
			struct brick_config *config, struct switch_error **errp)
{
	brick->burst = collect_burst;
	brick->burst_get = collect_burst_get;

	return 1;
}

static int collect_reset(struct brick *brick, struct switch_error **errp)
{
	enum side i;
	struct collect_state *state =
		brick_get_state(brick, struct collect_state);

	for (i = 0; i < MAX_SIDE; i++) {
		if (!state->pkts_mask[i])
			continue;

		packets_free(state->pkts[i], state->pkts_mask[i]);
		packets_forget(state->pkts[i], state->pkts_mask[i]);
		state->pkts_mask[i] = 0;
	}

	return 1;
}

static struct brick_ops collect_ops = {
	.name		= "collect",
	.state_size	= sizeof(struct collect_state),

	.init		= collect_init,

	.west_link	= brick_generic_west_link,
	.east_link	= brick_generic_east_link,
	.unlink		= brick_generic_unlink,

	.reset		= collect_reset,
};

brick_register(struct collect_state, &collect_ops);
