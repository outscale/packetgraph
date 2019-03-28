/* Copyright 2019 Outscale SAS
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
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "src/packets.h"

struct pg_vec {
	struct rte_mbuf *mbufs[PG_MAX_PKTS_BURST];
	int len;
};

struct pg_accumulator_config {
	enum pg_side output;
	uint64_t poll_min;
};

struct pg_accumulator_state {
	struct pg_brick brick;
	enum pg_side output;
	struct pg_vec burst_vec[PG_MAX_SIDE];
	uint16_t poll_count[PG_MAX_SIDE];
	uint16_t poll_min;
};

static int accumulator_burst(struct pg_brick *brick, enum pg_side from,
			     uint16_t edge_index, struct rte_mbuf **pkts,
			     uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_accumulator_state *state =
		pg_brick_get_state(brick, struct pg_accumulator_state);
	enum pg_side output = pg_flip_side(from);
	struct pg_brick_side *s = &brick->sides[output];
	struct pg_vec *vec = &state->burst_vec[output];

	PG_FOREACH_BIT(pkts_mask, it) {
		if (vec->len == PG_MAX_PKTS_BURST ||
		    state->poll_count[output] > state->poll_min) {
			pg_brick_burst(s->edge.link,
				       from, s->edge.pair_index,
				       vec->mbufs,
				       pg_mask_firsts(vec->len),
				       errp);
			if (state->poll_count[output] > state->poll_min) {
				state->poll_count[output] = 0;
				return 0;
			}
			vec->len = 0;
		}
		vec->mbufs[vec->len] = pkts[it];
		++vec->len;
	}
	return 0;
}

static int accumulator_poll(struct pg_brick *brick,
			    uint16_t *pkts_cnt,
			    struct pg_error **errp)
{
	struct pg_accumulator_state *state =
		pg_brick_get_state(brick, struct pg_accumulator_state);
	struct pg_brick_side *s =
		&brick->sides[state->output];
	uint64_t pkts_mask;
	bool ret = false;
	enum pg_side poll_dir = pg_flip_side(state->output);
	int i = state->output == PG_MAX_SIDE ? 0 : (int)poll_dir;

	if (state->output == PG_MAX_SIDE) {
		s = &brick->sides[PG_WEST_SIDE];
	} else if (state->poll_count[poll_dir] < state->poll_min) {
		++state->poll_count[poll_dir];
		return 0;
	}
	if (state->output == PG_MAX_SIDE) {
		if (state->poll_count[PG_WEST_SIDE] < state->poll_min) {
			++state->poll_count[PG_WEST_SIDE];
			ret = true;
		}
		if (state->poll_count[PG_EAST_SIDE] < state->poll_min) {
			++state->poll_count[PG_EAST_SIDE];
			ret = true;
		}
		if (ret)
			return 0;
	}
	do {
		++state->poll_count[i];
		if (state->output == PG_MAX_SIDE)
			s = &brick->sides[pg_flip_side(i)];
		pkts_mask = pg_mask_firsts(state->burst_vec[i].len);
		*pkts_cnt = state->burst_vec[i].len;
		pg_brick_burst(s->edge.link,
			       pg_flip_side(i), s->edge.pair_index,
			       state->burst_vec[i].mbufs,
			       pkts_mask, errp);
		pg_packets_free(state->burst_vec[i].mbufs,
				pkts_mask);
		state->burst_vec[i].len = 0;
		++i;
	} while (i < (int)state->output);
	state->poll_count[PG_WEST_SIDE] = 0;
	state->poll_count[PG_EAST_SIDE] = 0;

	return 0;
}

static int accumulator_init(struct pg_brick *brick,
			    struct pg_brick_config *config,
			    struct pg_error **errp)
{
	struct pg_accumulator_state *state;
	struct pg_accumulator_config *accumulator_config;

	state = pg_brick_get_state(brick, struct pg_accumulator_state);

	accumulator_config =
		(struct pg_accumulator_config *) config->brick_config;

	brick->burst = accumulator_burst;
	brick->poll = accumulator_poll;
	state->poll_min = accumulator_config->poll_min;
	state->output = accumulator_config->output;
	state->burst_vec[PG_WEST_SIDE].len = 0;
	state->burst_vec[PG_EAST_SIDE].len = 0;
	state->poll_count[PG_WEST_SIDE] = 0;
	state->poll_count[PG_EAST_SIDE] = 0;

	return 0;
}

static struct pg_brick_config *accumulator_config_new(const char *name,
						      enum pg_side output,
						      uint64_t poll_min)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_accumulator_config *accumulator_config =
		g_new0(struct pg_accumulator_config, 1);

	accumulator_config->output = output;
	accumulator_config->poll_min = poll_min;
	config->brick_config = (void *) accumulator_config;
	return pg_brick_config_init(config, name, 1, 1, PG_DIPOLE);
}

struct pg_brick *pg_accumulator_new(const char *name,
				    enum pg_side output,
				    uint64_t poll_min,
				    struct pg_error **errp)
{
	struct pg_brick_config *config =
		accumulator_config_new(name, output, poll_min);
	struct pg_brick *ret = pg_brick_new("accumulator", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static struct pg_brick_ops accumulator_ops = {
	.name		= "accumulator",
	.state_size	= sizeof(struct pg_accumulator_state),

	.init		= accumulator_init,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(accumulator, &accumulator_ops);
