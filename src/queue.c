/* Copyright 2016 Outscale SAS
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

#include <rte_config.h>
#include <packetgraph/packetgraph.h>
#include "utils/bitmask.h"
#include "brick-int.h"
#include "packets.h"

struct pg_queue_config {
	uint32_t rx_max_size;
};

struct pg_queue_state {
	struct pg_brick brick;
	/* side where packets are burst */
	enum pg_side output;
	/* maximal queue size */
	uint32_t rx_max_size;
	/* store bursted packets in queue */
	GAsyncQueue *rx;
	/* queue's friend */
	struct pg_queue_state *friend;
};

struct pg_queue_burst {
	struct rte_mbuf **pkts;
	uint64_t mask;
};

static struct pg_brick_config *queue_config_new(const char *name,
						uint32_t rx_max_size)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_queue_config *queue_config = g_new0(struct pg_queue_config,
						      1);

	queue_config->rx_max_size = rx_max_size;
	config->brick_config = (void *) queue_config;
	return pg_brick_config_init(config, name, 1, 1, PG_MONOPOLE);
}

uint8_t pg_queue_pressure(struct pg_brick *queue)
{
	struct pg_queue_state *state =
		pg_brick_get_state(queue, struct pg_queue_state);
	int queue_size = g_async_queue_length(state->rx);

	return queue_size <= 0 ? 0 : queue_size * 255 / state->rx_max_size;
}

static int queue_burst(struct pg_brick *brick, enum pg_side from,
		       uint16_t edge_index, struct rte_mbuf **pkts,
		       uint64_t pkts_mask, struct pg_error **error)
{
	struct pg_queue_state *state =
		pg_brick_get_state(brick, struct pg_queue_state);
	struct pg_queue_burst *burst = NULL;

	/* the oldest burst is throw away */
	if (g_async_queue_length(state->rx) >= (int) state->rx_max_size) {
		burst = g_async_queue_try_pop(state->rx);
		if (likely(burst != NULL)) {
			pg_packets_free(burst->pkts, burst->mask);
			g_free(burst);
		}
	}

	burst = g_new(struct pg_queue_burst, 1);
	burst->pkts = pkts;
	burst->mask = pkts_mask;
	pg_packets_incref(pkts, pkts_mask);
	g_async_queue_push(state->rx, burst);

#ifdef PG_QUEUE_BENCH
	struct pg_brick_side *side = &brick->side;

	if (side->burst_count_cb != NULL) {
		side->burst_count_cb(side->burst_count_private_data,
				     pg_mask_count(pkts_mask));
	}
#endif /* #ifdef PG_QUEUE_BENCH */
	return 0;
}

static int queue_poll(struct pg_brick *brick, uint16_t *pkts_cnt,
		      struct pg_error **error)
{
	int ret = 0;
	GAsyncQueue *tx = NULL;
	struct pg_queue_burst *burst = NULL;
	struct pg_queue_state *state =
		pg_brick_get_state(brick, struct pg_queue_state);
	struct pg_brick_side *s = &brick->side;

	/* Do we have a queue friend where to poll packets ? */
	if (!state->friend) {
		*pkts_cnt = 0;
		return 0;
	}

	tx = state->friend->rx;
	burst = g_async_queue_try_pop(tx);
	if (!burst) {
		*pkts_cnt = 0;
		return 0;
	}

	*pkts_cnt = pg_mask_count(burst->mask);
	ret = pg_brick_burst(s->edge.link, state->output,
			     s->edge.pair_index,
			     burst->pkts, burst->mask, error);

	pg_packets_free(burst->pkts, burst->mask);
	g_free(burst);
	return ret;
}

static int queue_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **error)
{
	struct pg_queue_state *state =
		pg_brick_get_state(brick, struct pg_queue_state);
	struct pg_queue_config *queue_config = config->brick_config;

	if (queue_config->rx_max_size == 0) {
		/* default queue size */
		queue_config->rx_max_size = 10;
	}

	state->rx = g_async_queue_new();
	if (state->rx == NULL) {
		*error = pg_error_new("Queue allocation failed");
		return -1;
	}
	state->rx_max_size = queue_config->rx_max_size;
	state->friend = NULL;
	brick->burst = queue_burst;
	brick->poll = queue_poll;
	return 0;
}

int pg_queue_friend(struct pg_brick *queue_1,
		    struct pg_brick *queue_2,
		    struct pg_error **error)
{
	struct pg_queue_state *state1 =
		pg_brick_get_state(queue_1, struct pg_queue_state);
	struct pg_queue_state *state2 =
		pg_brick_get_state(queue_2, struct pg_queue_state);

	if (state1->friend) {
		*error = pg_error_new("Queue %s (1st arg) already has a friend",
				      pg_brick_name(&state1->brick));
		return -1;
	}

	if (state2->friend) {
		*error = pg_error_new("Queue %s (2nd arg) already has a friend",
				      pg_brick_name(&state2->brick));
		return -1;
	}

	state1->friend = state2;
	state2->friend = state1;
	return 0;
}

bool pg_queue_are_friend(struct pg_brick *queue_1,
			 struct pg_brick *queue_2)
{
	return pg_queue_get_friend(queue_1) == queue_2;
}

struct pg_brick *pg_queue_get_friend(struct pg_brick *brick)
{
	struct pg_queue_state *state =
		pg_brick_get_state(brick, struct pg_queue_state);

	if (!state->friend)
		return NULL;
	return &(state->friend->brick);
}

static inline void unfriend(struct pg_queue_state *state)
{
	if (!state->friend)
		return;
	state->friend->friend = NULL;
	state->friend = NULL;
}

static inline void empty(struct pg_queue_state *state)
{
	struct pg_queue_burst *burst = NULL;
	GAsyncQueue *queue = state->rx;

	while ((burst = g_async_queue_try_pop(queue)) != NULL) {
		pg_packets_free(burst->pkts, burst->mask);
		g_free(burst);
	}
}

void pg_queue_unfriend(struct pg_brick *queue)
{
	unfriend(pg_brick_get_state(queue, struct pg_queue_state));
}

static void queue_destroy(struct pg_brick *brick, struct pg_error **error)
{
	struct pg_queue_state *state =
		pg_brick_get_state(brick, struct pg_queue_state);

	unfriend(state);
	empty(state);
	g_async_queue_unref(state->rx);
}

struct pg_brick *pg_queue_new(const char *name, int size,
			      struct pg_error **error)
{
	struct pg_brick_config *config = queue_config_new(name, size);
	struct pg_brick *ret = pg_brick_new("queue", config, error);

	pg_brick_config_free(config);
	return ret;
}

static void queue_link(struct pg_brick *brick, enum pg_side side, int edge)
{
	struct pg_queue_state *state =
		pg_brick_get_state(brick, struct pg_queue_state);
	/*
	 * We flip the side, because we don't want to flip side when
	 * we burst
	 */
	state->output = pg_flip_side(side);
}

static enum pg_side queue_get_side(struct pg_brick *brick)
{
	struct pg_queue_state *state;

	state = pg_brick_get_state(brick, struct pg_queue_state);
	return pg_flip_side(state->output);
}

static int queue_reset(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_queue_state *state =
		pg_brick_get_state(brick, struct pg_queue_state);

	unfriend(state);
	empty(state);
	return 0;
}

static struct pg_brick_ops queue_ops = {
	.name		= "queue",
	.state_size	= sizeof(struct pg_queue_state),
	.init		= queue_init,
	.destroy	= queue_destroy,
	.link_notify	= queue_link,
	.get_side	= queue_get_side,
	.unlink		= pg_brick_generic_unlink,
	.reset		= queue_reset,
};

pg_brick_register(queue, &queue_ops);
