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

static void realloc_pkts(struct rte_mbuf **packets)
{
	for (int i = 0; i < NB_PKTS; i++) {
		packets[i] = rte_pktmbuf_alloc(mp);
		g_assert(packets[i]);
		packets[i]->udata64 = i;
	}
}

static void accumulator_packets_poll(enum pg_side dir)
{
	struct pg_brick *acc, *gen, *col;
	struct pg_error *error = NULL;
	struct rte_mbuf *packets[NB_PKTS];
	struct rte_mbuf **res_pkts;
	uint16_t packet_count;
	uint64_t filtered_mask;

	memset(packets, 0, NB_PKTS * sizeof(struct rte_mbuf *));
	for (int i = 0; i < NB_PKTS; i++) {
		packets[i] = rte_pktmbuf_alloc(mp);
		g_assert(packets[i]);
		packets[i]->udata64 = i;
	}
	gen = pg_packetsgen_new("gen", 2, 2, pg_flip_side(dir),
				packets, NB_PKTS, &error);
	g_assert(!error);
	acc = pg_accumulator_new("acc", dir, 5, &error);
	g_assert(!error);
	col = pg_collect_new("col", &error);
	g_assert(!error);
	if (dir == PG_WEST_SIDE) {
		pg_brick_chained_links(&error, gen, acc, col);
		g_assert(!error);
		pg_brick_burst_to_east(acc, 0, packets,
				       pg_mask_firsts(NB_PKTS), &error);
		for (int i = 0; i < 6; i++) {
			pg_brick_poll(acc, &packet_count, &error);
			g_assert(!error);
		}
		res_pkts = pg_brick_west_burst_get(col, &filtered_mask, &error);
		g_assert(!error);
	} else {
		pg_brick_chained_links(&error, col, acc, gen);
		g_assert(!error);
		pg_brick_burst_to_west(acc, 0, packets,
				       pg_mask_firsts(NB_PKTS), &error);
		for (int i = 0; i < 6; i++) {
			pg_brick_poll(acc, &packet_count, &error);
			g_assert(!error);
		}
		res_pkts = pg_brick_east_burst_get(col, &filtered_mask, &error);
		g_assert(!error);
	}

	g_assert(res_pkts);
	g_assert(pg_mask_count(filtered_mask) == NB_PKTS);
	for (uint64_t i = 0; i < NB_PKTS; i++)
		g_assert(res_pkts[i]->udata64 == i % NB_PKTS);
	pg_brick_destroy(col);
	pg_brick_destroy(acc);
	pg_brick_destroy(gen);
}

static void test_accumulator_poll(void)
{
	accumulator_packets_poll(PG_WEST_SIDE);
	accumulator_packets_poll(PG_EAST_SIDE);
	accumulator_packets_poll(PG_MAX_SIDE);
}

uint64_t pg_res_id;

static void lanch_burst(struct pg_brick *acc,
			struct pg_brick *col,
			struct pg_brick *col2,
			struct rte_mbuf **packets,
			uint64_t expected,
			enum pg_side dir)
{
	struct rte_mbuf **result_pkts;
	uint64_t pkts_mask = 0;
	struct pg_error *error = NULL;
	uint64_t i;

	if (dir == PG_WEST_SIDE) {
		pg_brick_burst_to_west(acc, 0, packets,
				       pg_mask_firsts(NB_PKTS), &error);
		g_assert(!error);
		result_pkts = pg_brick_east_burst_get(col, &pkts_mask, &error);
	} else if (dir == PG_EAST_SIDE) {
		pg_brick_burst_to_east(acc, 0, packets,
				       pg_mask_firsts(NB_PKTS), &error);
		g_assert(!error);
		result_pkts = pg_brick_west_burst_get(col, &pkts_mask, &error);
	} else {
		pg_brick_burst_to_east(acc, 0, packets,
				       pg_mask_firsts(NB_PKTS), &error);
		result_pkts = pg_brick_west_burst_get(col2, &pkts_mask, &error);
		for (i = 0; i < expected; i++) {
			g_assert(result_pkts[i]->udata64 ==
				 (i + pg_res_id) % NB_PKTS);
		}
		pg_brick_burst_to_west(acc, 0, packets,
				       pg_mask_firsts(NB_PKTS), &error);
		result_pkts = pg_brick_east_burst_get(col, &pkts_mask, &error);
	}

	for (i = 0; i < expected; i++)
		g_assert(result_pkts[i]->udata64 == (i + pg_res_id) % NB_PKTS);

	if (expected)
		pg_res_id += (PG_MAX_PKTS_BURST % NB_PKTS);
}

static void accumulator_packets_burst(enum pg_side dir)
{
	struct pg_brick *acc, *col, *col2;
	struct pg_error *error = NULL;
	struct rte_mbuf *packets[NB_PKTS];
	int nb_in_acc = 0;

	pg_res_id = 0;
	memset(packets, 0, NB_PKTS * sizeof(struct rte_mbuf *));
	realloc_pkts(packets);
	g_assert(!error);
	acc = pg_accumulator_new("acc", dir, 5, &error);
	g_assert(!error);
	col = pg_collect_new("col", &error);
	g_assert(!error);
	col2 = pg_collect_new("col2", &error);
	g_assert(!error);
	if (dir == PG_EAST_SIDE)
		pg_brick_link(acc, col, &error);
	else if (dir == PG_WEST_SIDE)
		pg_brick_link(col, acc, &error);
	else
		pg_brick_chained_links(&error, col, acc, col2);
	g_assert(!error);
	for (int i = 0; i < 50; i++) {
		nb_in_acc += NB_PKTS;
		lanch_burst(acc, col, col2, packets,
			    nb_in_acc > PG_MAX_PKTS_BURST ?
			    PG_MAX_PKTS_BURST : 0, dir);
		if (nb_in_acc > PG_MAX_PKTS_BURST)
			nb_in_acc -= PG_MAX_PKTS_BURST;
		pg_packets_free(packets, pg_mask_firsts(NB_PKTS));
		realloc_pkts(packets);
	}

	pg_brick_destroy(col);
	pg_brick_destroy(col2);
	pg_brick_destroy(acc);
}

static void test_accumulator_burst(void)
{
	accumulator_packets_burst(PG_EAST_SIDE);
	accumulator_packets_burst(PG_EAST_SIDE);
	accumulator_packets_burst(PG_EAST_SIDE);
	accumulator_packets_burst(PG_EAST_SIDE);
	accumulator_packets_burst(PG_MAX_SIDE);
	accumulator_packets_burst(PG_EAST_SIDE);
	accumulator_packets_burst(PG_WEST_SIDE);
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
