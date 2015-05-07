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

#include "common.h"
#include "bricks/brick.h"
#include "utils/config.h"
#include "packets/packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "tests.h"

#define NB_PKTS 3

#define		TEST_HUB_DESTROY()				\
	do {							\
		brick_unlink(hub, &error);			\
		g_assert(!error);				\
		brick_unlink(collect_east1, &error);		\
		g_assert(!error);				\
		brick_unlink(collect_east2, &error);		\
		g_assert(!error);				\
		brick_unlink(collect_west1, &error);		\
		g_assert(!error);				\
		brick_unlink(collect_west2, &error);		\
		g_assert(!error);				\
		packets_free(pkts, mask_firsts(NB_PKTS));	\
		brick_decref(hub, &error);			\
		g_assert(!error);				\
		brick_reset(collect_east1, &error);		\
		g_assert(!error);				\
		brick_reset(collect_east2, &error);		\
		g_assert(!error);				\
		brick_reset(collect_west1, &error);		\
		g_assert(!error);				\
		brick_reset(collect_west2, &error);		\
		g_assert(!error);				\
		brick_decref(collect_east1, &error);		\
		g_assert(!error);				\
		brick_decref(collect_east2, &error);		\
		g_assert(!error);				\
		brick_decref(collect_west1, &error);		\
		g_assert(!error);				\
		brick_decref(collect_west2, &error);		\
		g_assert(!error);				\
		brick_config_free(config);			\
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
	struct brick *collect_east1, *collect_east2;
	struct brick *collect_west1, *collect_west2;
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
	collect_east1 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_east1);
	collect_east2 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_east2);
	collect_west1 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_west1);
	collect_west2 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_west2);
	brick_west_link(hub, collect_east1, &error);
	g_assert(!error);
	brick_west_link(hub, collect_east2, &error);
	g_assert(!error);
	brick_east_link(hub, collect_west1, &error);
	g_assert(!error);
	brick_east_link(hub, collect_west2, &error);
	g_assert(!error);

	/* send a burst to the east from */

	brick_burst_to_east(hub, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* check no packet ended on the sending block collect_east1 */
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect_east1, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect_east1, 0);

	/* check packets on collect_east2 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect_east2, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect_east2, NB_PKTS);

	/* check packets on collect_west1 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect_west1, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect_west1, 0);

	/* check packets on collect_west2 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect_west2, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect_west2, 0);


	TEST_HUB_DESTROY();
}

static void test_hub_west_dispatch(void)
{
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	struct brick *hub;
	struct brick *collect_east1, *collect_east2;
	struct brick *collect_west1, *collect_west2;
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
	collect_east1 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_east1);
	collect_east2 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_east2);
	collect_west1 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_west1);
	collect_west2 = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_west2);
	brick_west_link(hub, collect_east1, &error);
	g_assert(!error);
	brick_west_link(hub, collect_east2, &error);
	g_assert(!error);
	brick_east_link(hub, collect_west1, &error);
	g_assert(!error);
	brick_east_link(hub, collect_west2, &error);
	g_assert(!error);

	/* send a burst to the west from the hub */
	brick_burst_to_west(hub, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* check packets on collect_east1 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect_east1, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect_east1, 0);

	/* check packets on collect_east2 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect_east2, 0);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect_east2, NB_PKTS);

	/* check no packet ended on the sending block collect_west1 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect_west1, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect_west1, 0);

	/* check packets on collect_west2 */
	TEST_HUB_COLLECT_AND_TEST(brick_east_burst_get, collect_west2, NB_PKTS);
	TEST_HUB_COLLECT_AND_TEST(brick_west_burst_get, collect_west2, 0);

	TEST_HUB_DESTROY();
}

#undef TEST_HUB_INIT
#undef TEST_HUB_DESTROY

void test_hub(void)
{
	g_test_add_func("/brick/hub/east", test_hub_east_dispatch);
	g_test_add_func("/brick/hub/west", test_hub_west_dispatch);
}
