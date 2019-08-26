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
#include "utils/mempool.h"
#include "utils/network.h"

struct pg_rxtx_config {
	void *private_data;
	pg_rxtx_rx_callback_t rx;
	pg_rxtx_tx_callback_t tx;
};

struct pg_rxtx_state {
	struct pg_brick brick;
	/* packet graph internal side */
	enum pg_side output;
	void *private_data;
	pg_rxtx_rx_callback_t rx;
	pg_rxtx_tx_callback_t tx;
	pg_packet_t *rx_burst[PG_MAX_PKTS_BURST];
	pg_packet_t *tx_burst[PG_MAX_PKTS_BURST];
	uint64_t tx_bytes;
	uint64_t rx_bytes;
};

static struct pg_brick_config *rxtx_config_new(const char *name,
					       pg_rxtx_rx_callback_t rx,
					       pg_rxtx_tx_callback_t tx,
					       void *private_data)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_rxtx_config *rxtx_config = g_new0(struct pg_rxtx_config, 1);

	rxtx_config->private_data = private_data;
	rxtx_config->rx = rx;
	rxtx_config->tx = tx;
	config->brick_config = (void *) rxtx_config;
	return pg_brick_config_init(config, name, 1, 1, PG_MONOPOLE);
}

/* RX */
static int rxtx_burst(struct pg_brick *brick, enum pg_side from,
		      uint16_t edge_index, struct rte_mbuf **pkts,
		      uint64_t pkts_mask, struct pg_error **error)
{
	struct pg_rxtx_state *state =
		pg_brick_get_state(brick, struct pg_rxtx_state);

	if (!state->rx)
		return 0;
	uint64_t it_mask;
	uint16_t i;
	pg_packet_t **rx_burst = state->rx_burst;
	uint16_t cnt = 0;

	/* prepare RX packets */
	it_mask = pkts_mask;
	for (; it_mask;) {
		pg_low_bit_iterate(it_mask, i);
		rx_burst[cnt] = pkts[i];
		state->rx_bytes += pkts[i]->pkt_len;
		cnt++;
	}

	/* pass burst to user */
	state->rx(brick, rx_burst, cnt, state->private_data);

#ifdef PG_RXTX_BENCH
	struct pg_brick_side *side = &brick->side;

	if (side->burst_count_cb != NULL) {
		side->burst_count_cb(side->burst_count_private_data,
				     pg_mask_count(pkts_mask));
	}
#endif /* #ifdef PG_RXTX_BENCH */
	return 0;
}

/* TX */
static int rxtx_poll(struct pg_brick *brick, uint16_t *pkts_cnt,
		     struct pg_error **error)
{
	struct pg_rxtx_state *state =
		pg_brick_get_state(brick, struct pg_rxtx_state);
	struct rte_mbuf **tx_burst;

	if (!state->tx) {
		*pkts_cnt = 0;
		return 0;
	}
	struct pg_brick_side *s = &brick->side;
	uint16_t count = 0;

	tx_burst = state->tx_burst;
	/* let user write in packets */
	for (int i = 0; i < 64; ++i) {
		struct rte_mbuf *pkt = tx_burst[i];

		rte_pktmbuf_reset(pkt);
	}
	state->tx(brick, tx_burst, &count, state->private_data);
	*pkts_cnt = count;
	if (unlikely(count == 0))
		return 0;
	for (int i = 0; i < count; ++i) {
		pg_utils_guess_metadata(tx_burst[i]);
		state->tx_bytes += tx_burst[i]->pkt_len;
	}

	return pg_brick_burst(s->edge.link, state->output,
			      s->edge.pair_index,
			      tx_burst,
			      pg_mask_firsts(count), error);
}

static int rxtx_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **error)
{
	struct pg_rxtx_state *state =
		pg_brick_get_state(brick, struct pg_rxtx_state);
	struct pg_rxtx_config *rxtx_config = config->brick_config;
	struct rte_mempool *pool = pg_get_mempool();

	state->rx = rxtx_config->rx;
	state->tx = rxtx_config->tx;
	state->private_data = rxtx_config->private_data;
	brick->burst = rxtx_burst;
	if (state->tx) {
		brick->poll = rxtx_poll;
		/* pre-allocate packets */
		if (rte_pktmbuf_alloc_bulk(pool, state->tx_burst,
					   PG_MAX_PKTS_BURST) != 0) {
			*error = pg_error_new("RXTX allocation failed");
			return -1;
		}
	}
	return 0;
}

static void rxtx_destroy(struct pg_brick *brick, struct pg_error **error)
{
	struct pg_rxtx_state *state =
		pg_brick_get_state(brick, struct pg_rxtx_state);
	pg_packet_t **tx_burst = state->tx_burst;

	/* free pre-allocated packets */
	if (state->tx) {
		for (int i = 0; i < PG_MAX_PKTS_BURST; i++)
			rte_pktmbuf_free(tx_burst[i]);
	}
}

struct pg_brick *pg_rxtx_new(const char *name,
			     pg_rxtx_rx_callback_t rx,
			     pg_rxtx_tx_callback_t tx,
			     void *private_data)
{
	struct pg_brick_config *config = rxtx_config_new(name, rx, tx,
							 private_data);
	struct pg_error *error = NULL;
	struct pg_brick *ret = pg_brick_new("rxtx", config, &error);

	if (error)
		pg_error_free(error);
	pg_brick_config_free(config);
	return ret;
}

void *pg_rxtx_private_data(struct pg_brick *brick)
{
	return pg_brick_get_state(brick, struct pg_rxtx_state)->private_data;
}

static void rxtx_link(struct pg_brick *brick, enum pg_side side, int edge)
{
	struct pg_rxtx_state *state =
		pg_brick_get_state(brick, struct pg_rxtx_state);
	/*
	 * We flip the side, because we don't want to flip side when
	 * we burst
	 */
	state->output = pg_flip_side(side);
}

static enum pg_side rxtx_get_side(struct pg_brick *brick)
{
	struct pg_rxtx_state *state =
		pg_brick_get_state(brick, struct pg_rxtx_state);

	return pg_flip_side(state->output);
}

static uint64_t tx_bytes(struct pg_brick *brick)
{
	return pg_brick_get_state(brick, struct pg_rxtx_state)->tx_bytes;
}

static uint64_t rx_bytes(struct pg_brick *brick)
{
	return pg_brick_get_state(brick, struct pg_rxtx_state)->rx_bytes;
}

static struct pg_brick_ops rxtx_ops = {
	.name		= "rxtx",
	.state_size	= sizeof(struct pg_rxtx_state),
	.init		= rxtx_init,
	.destroy	= rxtx_destroy,
	.tx_bytes	= tx_bytes,
	.rx_bytes	= rx_bytes,
	.link_notify	= rxtx_link,
	.get_side	= rxtx_get_side,
	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(rxtx, &rxtx_ops);
