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
	struct switch_error *error = NULL;

	/* prepare the packets to send */
	for (i = 0; i < NB_PKTS; i++) {
		mbufs[i].udata64 = i;
		pkts[i] = &mbufs[i];
	}

	/* create a chain of a few nop brick with collectors on each sides */
	brick1 = brick_new("nop", config, &error);
	g_assert(!error);
	brick2 = brick_new("nop", config, &error);
	g_assert(!error);
	collect_west = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_west);
	collect_east = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_east);

	brick_link(collect_west, brick1, &error);
	g_assert(!error);
	brick_link(brick1, brick2, &error);
	g_assert(!error);
	brick_link(brick2, collect_east, &error);
	g_assert(!error);

	/* send a pkts to the west from the eastest nope brick */
	brick_burst_to_west(brick2, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* check pkts counter */
	g_assert(brick_pkts_count_get(collect_east, WEST_SIDE) == 0);
	g_assert(brick_pkts_count_get(collect_east, EAST_SIDE) == 0);
	g_assert(brick_pkts_count_get(collect_west, WEST_SIDE) == 3);
	g_assert(brick_pkts_count_get(collect_west, EAST_SIDE) == 0);
	g_assert(brick_pkts_count_get(brick1, WEST_SIDE) == 3);
	g_assert(brick_pkts_count_get(brick1, EAST_SIDE) == 0);
	g_assert(brick_pkts_count_get(brick2, WEST_SIDE) == 3);
	g_assert(brick_pkts_count_get(brick2, EAST_SIDE) == 0);

	/* check no packet ended on the east */
	result_pkts = brick_west_burst_get(collect_east, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect_east, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* collect pkts on the west */
	result_pkts = brick_west_burst_get(collect_west, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	result_pkts = brick_east_burst_get(collect_west, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	g_assert(result_pkts);
	for (i = 0; i < NB_PKTS; i++)
		g_assert(result_pkts[i]->udata64 == i);

	/* break the chain */
	brick_unlink(brick1, &error);
	g_assert(!error);
	brick_unlink(brick2, &error);
	g_assert(!error);
	brick_unlink(collect_west, &error);
	g_assert(!error);
	brick_unlink(collect_east, &error);
	g_assert(!error);

	/* destroy */
	brick_decref(brick1, &error);
	g_assert(!error);
	brick_decref(brick2, &error);
	g_assert(!error);
	brick_decref(collect_west, &error);
	g_assert(!error);
	brick_decref(collect_east, &error);
	g_assert(!error);

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
	struct switch_error *error = NULL;

	/* prepare the packets to send */
	for (i = 0; i < NB_PKTS; i++) {
		mbufs[i].udata64 = i;
		pkts[i] = &mbufs[i];
	}

	/* create a chain of a few nop brick with collectors on each sides */
	brick1 = brick_new("nop", config, &error);
	g_assert(!error);
	brick2 = brick_new("nop", config, &error);
	g_assert(!error);
	collect_west = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_west);
	collect_east = brick_new("collect", config, &error);
	g_assert(!error);
	g_assert(collect_east);

	brick_link(collect_west, brick1, &error);
	g_assert(!error);
	brick_link(brick1, brick2, &error);
	g_assert(!error);
	brick_link(brick2, collect_east, &error);
	g_assert(!error);

	/* send a pkts to the east from the westest nope brick */
	brick_burst_to_east(brick1, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* check pkts counter */
	g_assert(brick_pkts_count_get(collect_east, WEST_SIDE) == 0);
	g_assert(brick_pkts_count_get(collect_east, EAST_SIDE) == 3);
	g_assert(brick_pkts_count_get(collect_west, WEST_SIDE) == 0);
	g_assert(brick_pkts_count_get(collect_west, EAST_SIDE) == 0);
	g_assert(brick_pkts_count_get(brick1, WEST_SIDE) == 0);
	g_assert(brick_pkts_count_get(brick1, EAST_SIDE) == 3);
	g_assert(brick_pkts_count_get(brick2, WEST_SIDE) == 0);
	g_assert(brick_pkts_count_get(brick2, EAST_SIDE) == 3);


	/* check no packet ended on the west */
	result_pkts = brick_west_burst_get(collect_west, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect_west, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* collect pkts on the east */
	result_pkts = brick_east_burst_get(collect_east, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_west_burst_get(collect_east, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	g_assert(result_pkts);
	for (i = 0; i < NB_PKTS; i++)
		g_assert(result_pkts[i]->udata64 == i);

	/* break the chain */
	brick_unlink(brick1, &error);
	g_assert(!error);
	brick_unlink(brick2, &error);
	g_assert(!error);
	brick_unlink(collect_west, &error);
	g_assert(!error);
	brick_unlink(collect_east, &error);
	g_assert(!error);

	/* destroy */
	brick_decref(brick1, &error);
	g_assert(!error);
	brick_decref(brick2, &error);
	g_assert(!error);
	brick_decref(collect_west, &error);
	g_assert(!error);
	brick_decref(collect_east, &error);
	g_assert(!error);

	brick_config_free(config);
}

void test_brick_flow(void)
{
	/* tests in the same order as the header function declarations */
	g_test_add_func("/brick/flow/west", test_brick_flow_west);
	g_test_add_func("/brick/flow/east", test_brick_flow_east);
}
