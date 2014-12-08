/* Copyright 2014 Nodalink EURL
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

#include "switch-config.h"
#include "bricks/brick.h"
#include "utils/bitmask.h"
#include "utils/config.h"
#include "tests.h"

#define NB_PKTS 3

static void test_brick_flow_west(void)
{
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	struct brick *brick1, *brick2, *collect_west, *collect_east;
	struct rte_mbuf mbufs[NB_PKTS];
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[NB_PKTS];
	uint16_t i;
	uint64_t pkts_mask;

	/* prepare the packets to send */
	for (i = 0; i < NB_PKTS; i++) {
		mbufs[i].udata64 = i;
		pkts[i] = &mbufs[i];
	}

	/* create a chain of a few nop brick with collectors on each sides */
	brick1 = brick_new("nop", config);
	brick2 = brick_new("nop", config);
	collect_west = brick_new("collect", config);
	g_assert(collect_west);
	collect_east = brick_new("collect", config);
	g_assert(collect_east);

	brick_west_link(brick1, collect_west);
	brick_east_link(brick1, brick2);
	brick_east_link(brick2, collect_east);

	/* send a pkts to the west from the eastest nope brick */
	brick_burst_to_west(brick2, pkts, NB_PKTS, mask_firsts(NB_PKTS));

	/* check no packet ended on the east */
	result_pkts = brick_west_burst_get(collect_east, &pkts_mask);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect_east, &pkts_mask);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* collect pkts on the west */
	result_pkts = brick_west_burst_get(collect_west, &pkts_mask);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect_west, &pkts_mask);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	g_assert(result_pkts);
	for (i = 0; i < NB_PKTS; i++)
		g_assert(result_pkts[i]->udata64 == i);

	/* break the chain */
	brick_unlink(brick1);
	brick_unlink(brick2);
	brick_unlink(collect_west);
	brick_unlink(collect_east);

	/* destroy */
	brick_decref(brick1);
	brick_decref(brick2);
	brick_decref(collect_west);
	brick_decref(collect_east);

	brick_config_free(config);
}

static void test_brick_flow_east(void)
{
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	struct brick *brick1, *brick2, *collect_west, *collect_east;
	struct rte_mbuf mbufs[NB_PKTS];
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[NB_PKTS];
	uint16_t i;
	uint64_t pkts_mask;

	/* prepare the packets to send */
	for (i = 0; i < NB_PKTS; i++) {
		mbufs[i].udata64 = i;
		pkts[i] = &mbufs[i];
	}

	/* create a chain of a few nop brick with collectors on each sides */
	brick1 = brick_new("nop", config);
	brick2 = brick_new("nop", config);
	collect_west = brick_new("collect", config);
	g_assert(collect_west);
	collect_east = brick_new("collect", config);
	g_assert(collect_east);

	brick_west_link(brick1, collect_west);
	brick_east_link(brick1, brick2);
	brick_east_link(brick2, collect_east);

	/* send a pkts to the east from the westest nope brick */
	brick_burst_to_east(brick1, pkts, NB_PKTS, mask_firsts(NB_PKTS));

	/* check no packet ended on the west */
	result_pkts = brick_west_burst_get(collect_west, &pkts_mask);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect_west, &pkts_mask);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* collect pkts on the east */
	result_pkts = brick_east_burst_get(collect_east, &pkts_mask);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_west_burst_get(collect_east, &pkts_mask);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	g_assert(result_pkts);
	for (i = 0; i < NB_PKTS; i++)
		g_assert(result_pkts[i]->udata64 == i);

	/* break the chain */
	brick_unlink(brick1);
	brick_unlink(brick2);
	brick_unlink(collect_west);
	brick_unlink(collect_east);

	/* destroy */
	brick_decref(brick1);
	brick_decref(brick2);
	brick_decref(collect_west);
	brick_decref(collect_east);

	brick_config_free(config);
}

void test_brick_flow(void)
{
	/* tests in the same order as the header function declarations */
	g_test_add_func("/brick/flow/west", test_brick_flow_west);
	g_test_add_func("/brick/flow/east", test_brick_flow_east);
}
