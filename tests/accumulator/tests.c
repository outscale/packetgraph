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
#include "utils/tests.h"
#include <packetgraph/accumulator.h>
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "collect.h"
#include "packetsgen.h"

#define NB_PKTS 60

#define	BURST_SIDE(west_burst_fn, east_burst_fn) do {			\
		switch (dir) {						\
		case PG_WEST_SIDE:					\
			west_burst_fn(acc, 0, packets,			\
				pg_mask_firsts(NB_PKTS), &error);	\
			break ;						\
		case PG_EAST_SIDE:					\
			east_burst_fn(acc, 0, packets,			\
				pg_mask_firsts(NB_PKTS), &error);	\
			break ;						\
		case PG_MAX_SIDE:					\
			west_burst_fn(acc, 0, packets,			\
				pg_mask_firsts(NB_PKTS), &error);	\
			g_assert(!error);				\
			east_burst_fn(acc, 0, packets,			\
				pg_mask_firsts(NB_PKTS), &error);	\
			break ;						\
		}							\
		g_assert(!error);					\
	} while (0)

#define	CHECK_PKTS(expected_side, burst_fn, col) do {			\
		result_pkts = burst_fn(col, &pkts_mask, &error);	\
		if (is_expected_side(dir, expected_side)) {		\
			if (expected)					\
				g_assert(result_pkts);			\
			for (uint64_t i = 0; i < expected; i++)		\
				g_assert(result_pkts[i]->udata64 ==	\
					(i + pg_res_id) % NB_PKTS);	\
		}							\
	} while (0)

#define	INIT_TEST(loop) do {						\
		pg_res_id = 0;						\
		realloc_pkts(packets);					\
		acc = pg_accumulator_new("acc", dir, loop, &error);	\
		g_assert(!error);					\
		col1 = pg_collect_new("col1", &error);			\
		g_assert(!error);					\
		col2 = pg_collect_new("col2", &error);			\
		g_assert(!error);					\
		pg_brick_chained_links(&error, col1, acc, col2);	\
		g_assert(!error);					\
	} while (0)

uint64_t pg_res_id;

static bool is_expected_side(enum pg_side dir, enum pg_side expected)
{
	return (dir == expected || dir == PG_MAX_SIDE);
}

static void realloc_pkts(struct rte_mbuf **packets)
{
	for (int i = 0; i < NB_PKTS; i++) {
		packets[i] = rte_pktmbuf_alloc(mp);
		g_assert(packets[i]);
		packets[i]->udata64 = i;
	}
}

static void accumulator_packets_poll(enum pg_side dir, uint64_t loop)
{
	struct pg_brick *acc, *col1, *col2;
	struct pg_error *error = NULL;
	struct rte_mbuf *packets[NB_PKTS];
	struct rte_mbuf **result_pkts;
	uint16_t packet_count;
	uint64_t pkts_mask;
	uint64_t expected = NB_PKTS;

	INIT_TEST(loop);
	BURST_SIDE(pg_brick_burst_to_east, pg_brick_burst_to_west);
	for (uint64_t i = 0; i < loop + 1; i++) {
		pg_brick_poll(acc, &packet_count, &error);
		g_assert(!error);
	}
	CHECK_PKTS(PG_WEST_SIDE, pg_brick_west_burst_get, col1);
	if (dir == PG_EAST_SIDE)
		g_assert(!result_pkts);
	CHECK_PKTS(PG_EAST_SIDE, pg_brick_east_burst_get, col2);
	if (dir == PG_WEST_SIDE)
		g_assert(!result_pkts);

	pg_brick_destroy(col1);
	pg_brick_destroy(col2);
	pg_brick_destroy(acc);
}

static void test_accumulator_poll(void)
{
	accumulator_packets_poll(PG_WEST_SIDE, 5);
	accumulator_packets_poll(PG_EAST_SIDE, 0);
	accumulator_packets_poll(PG_MAX_SIDE, 23470);
}

static void accumulator_packets_burst(enum pg_side dir)
{
	struct pg_brick *acc, *col1, *col2;
	struct pg_error *error = NULL;
	struct rte_mbuf *packets[NB_PKTS];
	struct rte_mbuf **result_pkts;
	uint64_t pkts_mask = 0;
	uint64_t expected = 0;
	int acc_nb = 0;

	INIT_TEST(5);
	for (int i = 0; i < 42; i++) {
		acc_nb += NB_PKTS;
		expected = acc_nb > PG_MAX_PKTS_BURST ? PG_MAX_PKTS_BURST : 0;
		BURST_SIDE(pg_brick_burst_to_west, pg_brick_burst_to_east);
		CHECK_PKTS(PG_WEST_SIDE, pg_brick_east_burst_get, col1);
		CHECK_PKTS(PG_EAST_SIDE, pg_brick_west_burst_get, col2);
		if (acc_nb > PG_MAX_PKTS_BURST) {
			acc_nb -= PG_MAX_PKTS_BURST;
			pg_res_id += (PG_MAX_PKTS_BURST % NB_PKTS);
		}
		realloc_pkts(packets);
	}

	pg_brick_destroy(col1);
	pg_brick_destroy(col2);
	pg_brick_destroy(acc);
}

static void test_accumulator_burst(void)
{
	accumulator_packets_burst(PG_WEST_SIDE);
	accumulator_packets_burst(PG_EAST_SIDE);
	accumulator_packets_burst(PG_MAX_SIDE);
}

static void test_accumulator_life(void)
{
	struct pg_brick *acc;
	struct pg_error *error = NULL;

	acc = pg_accumulator_new("acc", PG_WEST_SIDE, 5, &error);
	g_assert(!error && acc);
	pg_brick_destroy(acc);
	acc = pg_accumulator_new("accuml", PG_EAST_SIDE, 5, &error);
	g_assert(!error && acc);
	pg_brick_destroy(acc);
}

static void test_accumulator(void)
{
	pg_test_add_func("/accumulator_life",
			 test_accumulator_life);
	pg_test_add_func("/accumulator_burst",
			 test_accumulator_burst);
	pg_test_add_func("/accumulator_poll",
			 test_accumulator_poll);
}

int main(int argc, char **argv)
{
	int r;

	g_test_init(&argc, &argv, NULL);
	g_assert(pg_start(argc, argv) >= 0);

	test_accumulator();
	r = g_test_run();

	pg_stop();
	return r;
}
