/* Copyright 2016 Outscale SAS

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

#include <glib.h>
#include <string.h>
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/mac.h"
#include "fail.h"
#include "collect.h"
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"

#define CHECK_ERROR(error) do {                 \
	if (error) {                      \
		pg_error_print(error);   \
		g_assert(!error);	\
	}                \
} while (0)

static void test_rxtx_lifecycle(void)
{
	struct pg_brick *brick;
	gchar *pd = g_strdup_printf("ho hai !");

	brick = pg_rxtx_new("rxtx", NULL, NULL, NULL);
	g_assert(brick);
	g_assert(pg_rxtx_private_data(brick) == NULL);
	pg_brick_destroy(brick);

	brick = pg_rxtx_new("rxtx", NULL, NULL, pd);
	g_assert(brick);
	g_assert(pg_rxtx_private_data(brick) == pd);
	pg_brick_destroy(brick);

	g_free(pd);
}

struct test_rxtx {
	uint8_t rx_data[PG_RXTX_MAX_TX_BURST_LEN];
	uint8_t tx_data[PG_RXTX_MAX_TX_BURST_LEN];
};

static void myrx(struct pg_brick *brick,
		 const struct pg_rxtx_packet *rx_burst,
		 uint16_t rx_burst_len,
		 void *private_data)
{
	struct test_rxtx *pd = (struct test_rxtx *)private_data;

	g_assert(brick);
	g_assert(rx_burst);
	g_assert(rx_burst_len == PG_RXTX_MAX_TX_BURST_LEN);
	g_assert(pd);
	for (int i = 0; i < rx_burst_len; i++) {
		g_assert(*rx_burst[i].len == 1);
		pd->rx_data[i] = rx_burst[i].data[0];
	}
}

static void mytx(struct pg_brick *brick,
		 struct pg_rxtx_packet *tx_burst,
		 uint16_t *tx_burst_len,
		 void *private_data)
{
	struct test_rxtx *pd = (struct test_rxtx *)private_data;

	g_assert(brick);
	g_assert(tx_burst);
	g_assert(*tx_burst_len == 0);
	g_assert(pd);
	*tx_burst_len = PG_RXTX_MAX_TX_BURST_LEN;
	for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++) {
		tx_burst[i].data[0] = pd->tx_data[i];
		*tx_burst[i].len = 1;
	}
}

static void test_rxtx_rx_to_tx(void)
{
	/* [tx]--[rx] */
	struct pg_error *error = NULL;
	struct pg_brick *btx, *brx;
	struct test_rxtx pd;
	struct pg_graph *g;

	brx = pg_rxtx_new("rx", &myrx, NULL, (void *) &pd);
	btx = pg_rxtx_new("tx", NULL, &mytx, (void *) &pd);
	pg_brick_link(brx, btx, &error);
	CHECK_ERROR(error);
	g = pg_graph_new("graph", brx, &error);
	CHECK_ERROR(error);
	g_assert(g);

	for (int r = 0; r < 1000; r++) {
		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++) {
			pd.tx_data[i] = i;
			pd.rx_data[i] = 0;
		}

		pg_graph_poll(g, &error);
		CHECK_ERROR(error);

		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++)
			g_assert(pd.rx_data[i] == i);

		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++) {
			pd.tx_data[i] = 63 - i;
			pd.rx_data[i] = 0;
		}

		pg_graph_poll(g, &error);
		CHECK_ERROR(error);

		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++)
			g_assert(pd.rx_data[i] == 63 - i);
	}
	pg_graph_destroy(g);
}

static void test_rxtx_rxtx_to_rxtx(void)
{
	/* [rxtx]--[rxtx] */
	struct pg_error *error = NULL;
	struct pg_brick *b1, *b2;
	struct test_rxtx pd1, pd2;
	struct pg_graph *g;

	b1 = pg_rxtx_new("1", &myrx, &mytx, (void *) &pd1);
	b2 = pg_rxtx_new("2", &myrx, &mytx, (void *) &pd2);
	pg_brick_link(b1, b2, &error);
	CHECK_ERROR(error);
	g = pg_graph_new("graph", b1, &error);
	CHECK_ERROR(error);
	g_assert(g);

	for (int r = 0; r < 1000; r++) {
		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++) {
			pd1.tx_data[i] = i;
			pd1.rx_data[i] = 0;
			pd2.tx_data[i] = 63 - i;
			pd2.rx_data[i] = 0;
		}

		pg_graph_poll(g, &error);
		CHECK_ERROR(error);

		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++) {
			g_assert(pd1.rx_data[i] == 63 - i);
			g_assert(pd2.rx_data[i] == i);
		}

		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++) {
			pd2.tx_data[i] = i;
			pd2.rx_data[i] = 0;
			pd1.tx_data[i] = 63 - i;
			pd1.rx_data[i] = 0;
		}

		pg_graph_poll(g, &error);
		CHECK_ERROR(error);

		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++) {
			g_assert(pd2.rx_data[i] == 63 - i);
			g_assert(pd1.rx_data[i] == i);
		}
	}
	pg_graph_destroy(g);
}
static void mytx_empty(struct pg_brick *brick,
		       struct pg_rxtx_packet *tx_burst,
		       uint16_t *tx_burst_len,
		       void *private_data)
{
	*tx_burst_len = 0;
}

static void test_rxtx_empty_burst(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *rxtx;
	struct pg_brick *fail;
	struct test_rxtx pd;
	struct pg_graph *g;

	rxtx = pg_rxtx_new("rxtx", NULL, &mytx_empty, (void *) &pd);
	CHECK_ERROR(error);
	fail = pg_fail_new("fail", &error);
	CHECK_ERROR(error);

	pg_brick_link(rxtx, fail, &error);
	CHECK_ERROR(error);

	g = pg_graph_new("graph", rxtx, &error);
	CHECK_ERROR(error);

	pg_graph_poll(g, &error);
	CHECK_ERROR(error);

	pg_graph_destroy(g);
}
int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int r;

	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	pg_start(argc, argv, &error);
	CHECK_ERROR(error);

	pg_test_add_func("/rxtx/lifecycle", test_rxtx_lifecycle);
	pg_test_add_func("/rxtx/rx_to_tx", test_rxtx_rx_to_tx);
	pg_test_add_func("/rxtx/rxtx_to_rxtx", test_rxtx_rxtx_to_rxtx);
	pg_test_add_func("/rxtx/rxtx_burst_not_propagate",
			test_rxtx_empty_burst);
	r = g_test_run();

	pg_stop();
	return r;
}
