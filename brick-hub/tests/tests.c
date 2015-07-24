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
#include <packetgraph/brick.h>
#include <packetgraph/packets.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/collect.h>


#define NB_PKTS 3

#define		TEST_HUB_DESTROY()					\
	do {								\
		brick_unlink(hub, &error);				\
		g_assert(!error);					\
		brick_unlink(collect1, &error);				\
		g_assert(!error);					\
		brick_unlink(collect2, &error);				\
		g_assert(!error);					\
		brick_unlink(collect3, &error);				\
		g_assert(!error);					\
		brick_unlink(collect4, &error);				\
		g_assert(!error);					\
		packets_free(pkts, mask_firsts(NB_PKTS));		\
		brick_decref(hub, &error);				\
		g_assert(!error);					\
		brick_reset(collect1, &error);				\
		g_assert(!error);					\
		brick_reset(collect2, &error);				\
		g_assert(!error);					\
		brick_reset(collect3, &error);				\
		g_assert(!error);					\
		brick_reset(collect4, &error);				\
		g_assert(!error);					\
		brick_decref(collect1, &error);				\
		g_assert(!error);					\
		brick_decref(collect2, &error);				\
		g_assert(!error);					\
		brick_decref(collect3, &error);				\
		g_assert(!error);					\
		brick_decref(collect4, &error);				\
		g_assert(!error);					\
		brick_config_free(config);				\
	} while (0)

#define		TEST_HUB_COLLECT_AND_TEST(get_burst_fn,	to_col, to_check) do { \
		result_pkts = get_burst_fn(to_col, &pkts_mask, &error); \
		g_assert(!error);					\
		g_assert(pkts_mask == mask_firsts(to_check));		\
		if (to_check)						\
			for (i = 0; i < NB_PKTS; i++)			\
				g_assert(result_pkts[i]->udata64 == i);	\
		else							\
			g_assert(!result_pkts);				\
	} while (0)

static void test_hub_east_dispatch(void)
{
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	struct brick *hub;
	struct brick *collect1, *collect2, *collect3, *collect4;
	struct rte_mbuf *pkts[NB_PKTS], **result_pkts;
	struct rte_mempool *mbuf_pool = get_mempool();
	struct switch_error *error = NULL;
	uint16_t i;
	uint64_t pkts_mask;

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
	}
	hub = hub_new ("myhub", 4, 4, &error);
	g_assert(!error);
	collect1 = collect_new("collect1", 4, 4, &error);
	g_assert(!error);
	g_assert(collect1);
	collect2 = collect_new("collect2", 4, 4, &error);
	g_assert(!error);
	g_assert(collect2);
	collect3 = collect_new("collect3", 4, 4, &error);
	g_assert(!error);
	g_assert(collect3);
	collect4 = collect_new("collect4", 4, 4, &error);
	g_assert(!error);
	g_assert(collect4);
	brick_link(collect1, hub, &error);
	g_assert(!error);
	brick_link(collect2, hub, &error);
	g_assert(!error);
	brick_link(hub, collect3, &error);
	g_assert(!error);
	brick_link(hub, collect4, &error);
	g_assert(!error);

	/* send a burst to the east from */

	brick_burst_to_east(hub, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);

	/* check no packet ended on the sending block collect1 */
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect1, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect1, 0);

	/* check packets on collect2 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect2, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect2, 0);

	/* check packets on collect3 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect3, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect3, NB_PKTS);

	/* check packets on collect4 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect4, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect4, NB_PKTS);


	TEST_HUB_DESTROY();
}

static void test_hub_west_dispatch(void)
{
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	struct brick *hub;
	struct brick *collect1, *collect2, *collect3, *collect4;
	struct rte_mbuf *pkts[NB_PKTS], **result_pkts;
	struct rte_mempool *mbuf_pool = get_mempool();
	struct switch_error *error = NULL;
	uint16_t i;
	uint64_t pkts_mask;

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
	}
	hub = brick_new("hub", config, &error);
	g_assert(!error);
	collect1 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect1);
	collect2 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect2);
	collect3 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect3);
	collect4 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect4);
	brick_link(collect1, hub, &error);
	g_assert(!error);
	brick_link(collect2, hub, &error);
	g_assert(!error);
	brick_link(hub, collect3, &error);
	g_assert(!error);
	brick_link(hub, collect4, &error);
	g_assert(!error);

	/* send a burst to the west from the hub */
	brick_burst_to_west(hub, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);

	/* check packets on collect1 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect1, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect1, 0);

	/* check packets on collect2 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect2, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect2, 0);

	/* check no packet ended on the sending block collect3 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect3, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect3, 0);

	/* check packets on collect4 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect4, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect4, NB_PKTS);

	TEST_HUB_DESTROY();
}

#undef TEST_HUB_INIT
#undef TEST_HUB_DESTROY

static void test_hub(void)
{
	g_test_add_func("/brick/hub/east", test_hub_east_dispatch);
	g_test_add_func("/brick/hub/west", test_hub_west_dispatch);
}

int main(int argc, char **argv)
{
	struct switch_error *error;
	int r;

	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	packetgraph_start(argc, argv, &error);
	g_assert(!error);

	test_hub();
	r = g_test_run();

	packetgraph_stop();
	return r;
}
