/* Copyright 2016 Outscale SAS
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

#include <rte_config.h>
#include <packetgraph/packetgraph.h>
#include "utils/bitmask.h"
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"

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
	struct pg_rxtx_packet rx_burst[PG_MAX_PKTS_BURST];
	struct pg_rxtx_packet tx_burst[PG_MAX_PKTS_BURST];
	struct rte_mbuf *tx_mbuf[PG_MAX_PKTS_BURST];
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
	struct rte_mbuf *tmp;
	struct pg_rxtx_packet *rx_burst = state->rx_burst;
	uint16_t cnt = 0;

	/* prepare RX packets */
	it_mask = pkts_mask;
	for (; it_mask;) {
		pg_low_bit_iterate(it_mask, i);
		tmp = pkts[i];
		rx_burst[cnt].data = rte_pktmbuf_mtod(tmp, uint8_t *);
		rx_burst[cnt].len = &tmp->data_len;
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

	if (!state->tx) {
		*pkts_cnt = 0;
		return 0;
	}
	struct pg_rxtx_packet *tx_burst = state->tx_burst;
	struct rte_mbuf **mbuf = state->tx_mbuf;
	struct pg_brick_side *s = &brick->side;
	uint16_t count = 0;

	/* let user write in packets */
	state->tx(brick, tx_burst, &count, state->private_data);
	*pkts_cnt = count;
	if (unlikely(count == 0))
		return 0;
	return pg_brick_burst(s->edge.link, state->output,
			      s->edge.pair_index,
			      mbuf, pg_mask_firsts(count), error);
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
		if (rte_pktmbuf_alloc_bulk(pool, state->tx_mbuf,
					   PG_MAX_PKTS_BURST) != 0) {
			*error = pg_error_new("RXTX allocation failed");
			return -1;
		}
		/* pre-configure user packets pointers */
		struct pg_rxtx_packet *tx_burst = state->tx_burst;
		struct rte_mbuf **tmp = state->tx_mbuf;

		for (int i = 0; i < PG_MAX_PKTS_BURST; i++) {
			tx_burst[i].data = rte_pktmbuf_mtod(tmp[i], uint8_t *);
			tx_burst[i].len = &tmp[i]->data_len;
		}
	}
	return 0;
}

static void rxtx_destroy(struct pg_brick *brick, struct pg_error **error)
{
	struct pg_rxtx_state *state =
		pg_brick_get_state(brick, struct pg_rxtx_state);

	/* free pre-allocated packets */
	if (state->tx) {
		struct rte_mbuf **tx_mbuf = state->tx_mbuf;

		for (int i = 0; i < PG_MAX_PKTS_BURST; i++)
			rte_pktmbuf_free(tx_mbuf[i]);
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

static struct pg_brick_ops rxtx_ops = {
	.name		= "rxtx",
	.state_size	= sizeof(struct pg_rxtx_state),
	.init		= rxtx_init,
	.destroy	= rxtx_destroy,
	.link_notify	= rxtx_link,
	.get_side	= rxtx_get_side,
	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(rxtx, &rxtx_ops);
