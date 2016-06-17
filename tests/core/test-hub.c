/* Copyright 2014 Outscale SAS
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
#include <rte_config.h>
#include <rte_mbuf.h>
#include <string.h>
#include <packetgraph/packetgraph.h>
#include <packetgraph/hub.h>
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "collect.h"
#include "tests.h"

#define NB_PKTS 3

#define		TEST_HUB_DESTROY()					\
	do {								\
		pg_brick_unlink(hub, &error);				\
		g_assert(!error);					\
		pg_brick_unlink(collect0_1, &error);			\
		g_assert(!error);					\
		pg_brick_unlink(collect0_2, &error);			\
		g_assert(!error);					\
		pg_brick_unlink(collect1_1, &error);			\
		g_assert(!error);					\
		pg_brick_unlink(collect1_2, &error);			\
		g_assert(!error);					\
		pg_packets_free(pkts, pg_mask_firsts(NB_PKTS));		\
		pg_brick_decref(hub, &error);				\
		g_assert(!error);					\
		g_assert(pg_brick_reset(collect0_1, &error) == 0);	\
		g_assert(!error);					\
		g_assert(pg_brick_reset(collect0_2, &error) == 0);	\
		g_assert(!error);					\
		g_assert(pg_brick_reset(collect1_1, &error) == 0);	\
		g_assert(!error);					\
		g_assert(pg_brick_reset(collect1_2, &error) == 0);	\
		g_assert(!error);					\
		pg_brick_decref(collect0_1, &error);			\
		g_assert(!error);					\
		pg_brick_decref(collect0_2, &error);			\
		g_assert(!error);					\
		pg_brick_decref(collect1_1, &error);			\
		g_assert(!error);					\
		pg_brick_decref(collect1_2, &error);			\
		g_assert(!error);					\
	} while (0)

#define		TEST_HUB_COLLECT_AND_TEST(get_burst_fn,	to_col, to_check) do { \
		result_pkts = get_burst_fn(to_col, &pkts_mask, &error); \
		g_assert(!error);					\
		g_assert(pkts_mask == pg_mask_firsts(to_check));	\
		if (to_check)						\
			for (i = 0; i < NB_PKTS; i++)			\
				g_assert(result_pkts[i]->udata64 == i);	\
		else							\
			g_assert(!result_pkts);				\
	} while (0)

static void test_hub_east_dispatch(void)
{
	struct pg_brick *hub;
	struct pg_brick *collect0_1, *collect0_2, *collect1_1, *collect1_2;
	struct rte_mbuf *pkts[NB_PKTS], **result_pkts;
	struct rte_mempool *mbuf_pool = pg_get_mempool();
	struct pg_error *error = NULL;
	uint16_t i;
	uint64_t pkts_mask;

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
	}
	hub = pg_hub_new("myhub", 4, 4, &error);
	g_assert(!error);
	collect0_1 = pg_collect_new("collect0_1", &error);
	g_assert(!error);
	g_assert(collect0_1);
	collect0_2 = pg_collect_new("collect0_2", &error);
	g_assert(!error);
	g_assert(collect0_2);
	collect1_1 = pg_collect_new("collect1_1", &error);
	g_assert(!error);
	g_assert(collect1_1);
	collect1_2 = pg_collect_new("collect1_2", &error);
	g_assert(!error);
	g_assert(collect1_2);
	pg_brick_link(collect0_1, hub, &error);
	g_assert(!error);
	pg_brick_link(collect0_2, hub, &error);
	g_assert(!error);
	pg_brick_link(hub, collect1_1, &error);
	g_assert(!error);
	pg_brick_link(hub, collect1_2, &error);
	g_assert(!error);

	/* send a burst to the east from */

	pg_brick_burst_to_east(hub, 0, pkts, pg_mask_firsts(NB_PKTS),
			    &error);

	/* check no packet ended on the sending block collect1 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect0_1, 0);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect0_1, 0);

	/* check packets on collect2 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect0_2, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect0_2, 0);

	/* check packets on collect3 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect1_1, 0);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect1_1, NB_PKTS);

	/* check packets on collect4 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect1_2, 0);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect1_2, NB_PKTS);


	TEST_HUB_DESTROY();
}

static void test_hub_west_dispatch(void)
{
	struct pg_brick *hub;
	struct pg_brick *collect0_1, *collect0_2, *collect1_1, *collect1_2;
	struct rte_mbuf *pkts[NB_PKTS], **result_pkts;
	struct rte_mempool *mbuf_pool = pg_get_mempool();
	struct pg_error *error = NULL;
	uint16_t i;
	uint64_t pkts_mask;

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
	}
	hub = pg_hub_new("hub", 4, 4, &error);
	g_assert(!error);
	collect0_1 = pg_collect_new("collect", &error);
	g_assert(!error);
	g_assert(collect0_1);
	collect0_2 = pg_collect_new("collect", &error);
	g_assert(!error);
	g_assert(collect0_2);
	collect1_1 = pg_collect_new("collect", &error);
	g_assert(!error);
	g_assert(collect1_1);
	collect1_2 = pg_collect_new("collect", &error);
	g_assert(!error);
	g_assert(collect1_2);
	pg_brick_link(collect0_1, hub, &error);
	g_assert(!error);
	pg_brick_link(collect0_2, hub, &error);
	g_assert(!error);
	pg_brick_link(hub, collect1_1, &error);
	g_assert(!error);
	pg_brick_link(hub, collect1_2, &error);
	g_assert(!error);

	/* send a burst to the west from the hub */
	pg_brick_burst_to_west(hub, 0, pkts, pg_mask_firsts(NB_PKTS),
			    &error);

	/* check packets on collect1 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect0_1, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect0_1, 0);

	/* check packets on collect2 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect0_2, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect0_2, 0);

	/* check no packet ended on the sending block collect3 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect1_1, 0);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect1_1, 0);

	/* check packets on collect4 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect1_2, 0);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect1_2, NB_PKTS);

	TEST_HUB_DESTROY();
}

/*Test, allow create multipole with a null side*/

static void test_hub_one_side_null(void)
{
	struct pg_brick *collect0_1, *collect0_2, *collect1_1, *collect1_2,
	*collect2_1, *collect2_2;
	struct rte_mempool *mbuf_pool = pg_get_mempool();
	struct rte_mbuf *pkts[NB_PKTS], **result_pkts;
	struct pg_brick *hub, *hub1, *hub2;
	struct pg_error *error = NULL;
	uint64_t pkts_mask;
	uint16_t i;

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
	}

	hub = pg_hub_new("myhub", 4, 0, &error);
	g_assert(!error);
	g_assert(hub);
	hub1 = pg_hub_new("myhub1", 0, 4, &error);
	g_assert(!error);
	g_assert(hub1);
	hub2 = pg_hub_new("myhub2", 0, 0, &error);
	g_assert(error);
	g_assert(!hub2);
	pg_error_free(error);
 	error = NULL;

	collect0_1 = pg_collect_new("collect0_1", &error);
	g_assert(!error);
	g_assert(collect0_1);

        collect0_2 = pg_collect_new("collect0_2", &error);
        g_assert(!error);
        g_assert(collect0_2);

	collect1_1 = pg_collect_new("collect1_1", &error);
	g_assert(!error);
	g_assert(collect1_1);

        collect1_2 = pg_collect_new("collect1_2", &error);
        g_assert(!error);
        g_assert(collect1_2);

	collect2_1 = pg_collect_new("collect2_1", &error);
	g_assert(!error);
	g_assert(collect2_1);

	collect2_2 = pg_collect_new("collect2_2", &error);
	g_assert(!error);
	g_assert(collect2_2);;
	
	pg_brick_link(hub, collect0_1, &error);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	pg_brick_link(hub, collect0_2, &error);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	pg_brick_link(collect0_1, hub, &error);
	g_assert(!error);
	pg_brick_link(collect0_2, hub, &error);
	g_assert(!error);

	pg_brick_link(collect1_1, hub1, &error);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	pg_brick_link(collect1_2, hub1, &error);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	pg_brick_link(hub1, collect1_1, &error);
	g_assert(!error);
	pg_brick_link(hub1, collect1_2, &error);
	g_assert(!error);

	pg_brick_link(collect2_1, hub2, &error);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	pg_brick_link(collect2_2, hub2, &error);
	g_assert(error);
	pg_error_free(error);
	error =NULL;
	pg_brick_link(hub2, collect2_1, &error);
	g_assert(error);
	g_free(error);
	pg_brick_link(hub2, collect2_1, &error);
	g_assert(error);
	pg_error_free(error);
	error = NULL;

	/* send a burst to the west from the hub */
	pg_brick_burst_to_west(hub, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	g_assert(!error);

	/* send a burst to the east from */
	pg_brick_burst_to_east(hub, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	g_assert(!error);

	/* check packets on collect0_1 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect0_1, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect0_1, 0);

	/* check packets on collect0_2 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect0_2, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect0_2, 0);

	/* check no packet ended on the sending block collect2_1 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect2_1, 0);
	TEST_HUB_COLLECT_AND_TEST(pg_brick_west_burst_get, collect2_1, 0);

	/* check packets on collect2_2 */
	TEST_HUB_COLLECT_AND_TEST(pg_brick_east_burst_get, collect2_2, 0);

	pg_brick_unlink(hub1, &error);
	g_assert(!error);

	TEST_HUB_DESTROY();
        pg_brick_unlink(collect2_1, &error);            
        g_assert(!error);
        pg_brick_unlink(collect2_2, &error);
        g_assert(!error);
        pg_brick_reset(collect2_1, &error);
        g_assert(!error);
        pg_brick_reset(collect2_2, &error);
        g_assert(!error);
        pg_brick_decref(collect2_1, &error);
        g_assert(!error);
        pg_brick_decref(collect2_2, &error);
        g_assert(!error);                        
}

#undef TEST_HUB_INIT
#undef TEST_HUB_DESTROY

void test_hub(void)
{
	g_test_add_func("/hub/east", test_hub_east_dispatch);
	g_test_add_func("/hub/west", test_hub_west_dispatch);
	g_test_add_func("/hub/test_side_null", test_hub_one_side_null);
}
