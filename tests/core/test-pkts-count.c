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

#include <glib.h>
#include <string.h>

#include <packetgraph/common.h>
#include <packetgraph/nop.h>
#include "brick-int.h"
#include "utils/bitmask.h"
#include "utils/config.h"
#include "utils/mempool.h"
#include "packets.h"
#include "collect.h"
#include "tests.h"

#define NB_PKTS 3
#define NB_LOOP 4

#define	TEST_PKTS_COUNT_INIT()						\
	struct pg_brick *brick, *collect_west, *collect_east;		\
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST], **result_pkts;	\
	struct rte_mempool *mp = pg_get_mempool();			\
	struct pg_error *error = NULL;					\
	uint16_t i;							\
	uint64_t pkts_mask;						\
	memset(pkts, 0, PG_MAX_PKTS_BURST * sizeof(struct rte_mbuf *));	\
	for (i = 0; i < NB_PKTS; i++) {					\
		pkts[i] = rte_pktmbuf_alloc(mp);			\
		g_assert(pkts[i]);					\
		pkts[i]->udata64 = i;					\
	}								\
	brick = pg_nop_new("nop", &error);				\
	g_assert(!error);						\
	collect_west = pg_collect_new("mybrick-w", &error);		\
	g_assert(!error);						\
	g_assert(collect_west);						\
	collect_east = pg_collect_new("mybrick-e", &error);		\
	g_assert(!error);						\
	g_assert(collect_east);						\
	pg_brick_link(collect_west, brick, &error);			\
	g_assert(!error);						\
	pg_brick_link(brick, collect_east, &error);			\
	g_assert(!error)

#define	TEST_PKTS_COUNT_DESTROY() do {				\
		pg_brick_unlink(brick, &error);			\
		g_assert(!error);				\
		pg_brick_unlink(collect_west, &error);		\
		g_assert(!error);				\
		pg_brick_unlink(collect_east, &error);		\
		g_assert(!error);				\
		pg_packets_free(pkts, pg_mask_firsts(NB_PKTS));	\
		pg_brick_decref(brick, &error);			\
		g_assert(!error);				\
		pg_brick_decref(collect_west, &error);		\
		g_assert(!error);				\
		pg_brick_decref(collect_east, &error);		\
		g_assert(!error);				\
	} while (0)

#define	TEST_PKTS_COUNT_CHECK(get_burst_fn, to_collect, result_to_check) do { \
		result_pkts = get_burst_fn(to_collect, &pkts_mask, &error); \
		g_assert(!error);					\
		g_assert(pkts_mask == pg_mask_firsts(result_to_check));	\
		if (result_to_check)					\
			for (i = 0; i < NB_PKTS; i++)			\
				g_assert(result_pkts[i]->udata64 == i);	\
		else							\
			g_assert(!result_pkts);				\
	} while (0)


static void test_brick_pkts_count_west(void)
{
	TEST_PKTS_COUNT_INIT();
	unsigned int pkts_count;
	int j;

	for (j = 0, pkts_count = NB_PKTS; j < NB_LOOP;
	     ++j, pkts_count += NB_PKTS) {
		pg_brick_burst_to_west(brick, 0, pkts,
				       pg_mask_firsts(NB_PKTS), &error);
		g_assert(!error);
		g_assert(pg_brick_pkts_count_get(collect_east, WEST_SIDE) == 0);
		g_assert(pg_brick_pkts_count_get(collect_east, EAST_SIDE) == 0);
		g_assert(pg_brick_pkts_count_get(collect_west,
					      WEST_SIDE) == pkts_count);
		g_assert(pg_brick_pkts_count_get(collect_west, EAST_SIDE) == 0);
		g_assert(pg_brick_pkts_count_get(brick, WEST_SIDE) == pkts_count);
		g_assert(pg_brick_pkts_count_get(brick, EAST_SIDE) == 0);

		TEST_PKTS_COUNT_CHECK(pg_brick_west_burst_get, collect_west, 0);
		TEST_PKTS_COUNT_CHECK(pg_brick_east_burst_get, collect_west, 3);
		TEST_PKTS_COUNT_CHECK(pg_brick_west_burst_get, collect_east, 0);
		TEST_PKTS_COUNT_CHECK(pg_brick_east_burst_get, collect_east, 0);
	}
	TEST_PKTS_COUNT_DESTROY();
}

static void test_brick_pkts_count_east(void)
{
	TEST_PKTS_COUNT_INIT();
	unsigned int pkts_count;
	int j;

	for (j = 0, pkts_count = NB_PKTS; j < NB_LOOP;
	     ++j, pkts_count += NB_PKTS) {
		pg_brick_burst_to_east(brick, 0, pkts,
				    pg_mask_firsts(NB_PKTS), &error);
		g_assert(!error);
		g_assert(pg_brick_pkts_count_get(collect_east, WEST_SIDE) == 0);
		g_assert(pg_brick_pkts_count_get(collect_east,
					      EAST_SIDE) == pkts_count);
		g_assert(pg_brick_pkts_count_get(collect_west, WEST_SIDE) == 0);
		g_assert(pg_brick_pkts_count_get(collect_west, EAST_SIDE) == 0);
		g_assert(pg_brick_pkts_count_get(brick, WEST_SIDE) == 0);
		g_assert(pg_brick_pkts_count_get(brick, EAST_SIDE) == pkts_count);

		TEST_PKTS_COUNT_CHECK(pg_brick_west_burst_get, collect_west, 0);
		TEST_PKTS_COUNT_CHECK(pg_brick_east_burst_get, collect_west, 0);
		TEST_PKTS_COUNT_CHECK(pg_brick_west_burst_get, collect_east, 3);
		TEST_PKTS_COUNT_CHECK(pg_brick_east_burst_get, collect_east, 0);
	}
	TEST_PKTS_COUNT_DESTROY();
}

#undef	TEST_PKTS_COUNT_CHECK
#undef	TEST_PKTS_COUNT_INIT
#undef	TEST_PKTS_COUNT_DESTROY

void test_pkts_count(void)
{
	/* tests in the same order as the header function declarations */
	g_test_add_func("/core/pkts-counter/west", test_brick_pkts_count_west);
	g_test_add_func("/core/pkts-counter/east", test_brick_pkts_count_east);
}
