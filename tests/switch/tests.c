/* Copyright 2014 Nodalink EURL

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
#include "utils/malloc.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/mac.h"
#include "collect.h"
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include <packetgraph/nop.h>
#include <packetgraph/switch.h>

#define NB_PKTS          3

#define TEST_PORTS	40
#define SIDE_PORTS	(TEST_PORTS / 2)
#define TEST_PKTS	8000
#define TEST_MOD	(TEST_PKTS - PG_MAX_PKTS_BURST)
#define TEST_PKTS_COUNT (200 * 1000 * 1000)
#define PKTS_BURST	3

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)

struct rte_mempool *mbuf_pool;

static void test_switch_lifecycle(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick;

	brick = pg_switch_new("switch", 20, 20, PG_DEFAULT_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	pg_brick_decref(brick, &error);
	CHECK_ERROR(error);
	pg_malloc_should_fail = 1;
	brick = pg_switch_new("switch", 20, 20, PG_DEFAULT_SIDE, &error);
	pg_malloc_should_fail = 0;
	g_assert(error && error->err_no == ENOMEM);
	g_assert(!brick);
	pg_error_free(error);
}

static void test_switch_learn(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick, *collect1, *collect2, *collect3;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = pg_switch_new("switch", 4, 4, PG_DEFAULT_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	collect1 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	collect2 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	collect3 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	pg_brick_link(collect1, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(collect2, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(brick, collect3, &error);
	CHECK_ERROR(error);

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "F0:F1:F2:F3:F4:F5", "E0:E1:E2:E3:E4:E5");
	}

	/* Send the packet through the port 0 of  switch it should been
	 * forwarded on all port because of the learning mode.
	 */
	pg_brick_burst_to_east(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);

	/* Now send responses packets (source and dest mac address swapped) */
	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "E0:E1:E2:E3:E4:E5", "F0:F1:F2:F3:F4:F5");
	}

	/* through port 0 of east side of the switch */
	pg_brick_burst_to_west(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the switch should not mass learn forward */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have ended in collect 1 (port 0 of west side) */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);

	/* Now the conversation can continue */
	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "F0:F1:F2:F3:F4:F5", "E0:E1:E2:E3:E4:E5");
	}

	pg_brick_burst_to_east(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the switch should not mass learn forward */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have ended in collect 3 (port 0 of east side) */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);

	pg_malloc_should_fail = 1;
	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "F0:F1:A2:F3:F4:F5", "A0:A1:A2:A3:A4:A5");
	}
	g_assert(pg_brick_burst_to_east(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
					&error) < 0);
	g_assert(error && error->err_no == ENOMEM);
	pg_error_free(error);
	error = NULL;

	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	pg_malloc_should_fail = 0;
	g_assert(!pg_brick_burst_to_east(brick, 0, pkts,
					 pg_mask_firsts(NB_PKTS),
					 &error));

	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(pkts_mask);
	g_assert(result_pkts);
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(pkts_mask);
	g_assert(result_pkts);


	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the chain */
	pg_brick_unlink(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect3, &error);
	CHECK_ERROR(error);

	pg_brick_decref(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect3, &error);
	CHECK_ERROR(error);
	pg_brick_decref(brick, &error);

	CHECK_ERROR(error);
}

static void test_switch_switching(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick, *collect1, *collect2, *collect3, *collect4;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = pg_switch_new("switch", 4, 4, PG_WEST_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	collect1 = pg_collect_new("collect1", &error);
	CHECK_ERROR(error);

	collect2 = pg_collect_new("collect2", &error);
	CHECK_ERROR(error);

	collect3 = pg_collect_new("collect3", &error);
	CHECK_ERROR(error);

	collect4 = pg_collect_new("collect4", &error);
	CHECK_ERROR(error);

	pg_brick_link(collect1, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(collect2, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(brick, collect3, &error);
	CHECK_ERROR(error);
	pg_brick_link(brick, collect4, &error);
	CHECK_ERROR(error);

	/* we will first exercise the learning abilities of the switch by
	 * sending multiple packets bursts with different source addresses
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "10:10:10:10:10:10", "AA:AA:AA:AA:AA:AA");
	}
	/* west port 0 == collect 1 */
	pg_brick_burst_to_east(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect4, &error) == 0);
	CHECK_ERROR(error);

	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "22:22:22:22:22:22", "AA:AA:AA:AA:AA:AA");
	}
	/* west port 1 == collect 2 */
	pg_brick_burst_to_east(brick, 1, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect4, &error) == 0);
	CHECK_ERROR(error);

	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "66:66:66:66:66:66", "AA:AA:AA:AA:AA:AA");
	}

	/* east port 0 == collect 3 */
	pg_brick_burst_to_west(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect4, &error) == 0);
	CHECK_ERROR(error);

	pg_set_mac_addrs(pkts[0], "AA:AA:AA:AA:AA:AA",
			 "10:10:10:10:10:10");
	pg_set_mac_addrs(pkts[1], "AA:AA:AA:AA:AA:AA",
			 "22:22:22:22:22:22");
	pg_set_mac_addrs(pkts[2], "AA:AA:AA:AA:AA:AA",
			 "66:66:66:66:66:66");

	/* east port 1 == collect 4 */
	pg_brick_burst_to_west(brick, 1, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == 0x1);
	g_assert(result_pkts[0]);
	g_assert(result_pkts[0]->udata64 == 0);

	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == 0x2);
	g_assert(result_pkts[1]);
	g_assert(result_pkts[1]->udata64 == 1);

	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == 0x4);
	g_assert(result_pkts[2]);
	g_assert(result_pkts[2]->udata64 == 2);

	result_pkts = pg_brick_west_burst_get(collect4, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* potentially badly addressed packets */
	result_pkts = pg_brick_west_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_west_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_east_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_east_burst_get(collect4, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);
	pg_brick_unlink(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect3, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect4, &error);
	CHECK_ERROR(error);

	pg_brick_decref(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect3, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect4, &error);
	CHECK_ERROR(error);
	pg_brick_decref(brick, &error);
	CHECK_ERROR(error);
}

static void test_switch_unlink(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick, *collect1, *collect2, *collect3, *collect4;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = pg_switch_new("switch", 4, 4, PG_WEST_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	collect1 = pg_collect_new("collect1", &error);
	CHECK_ERROR(error);

	collect2 = pg_collect_new("collect2", &error);
	CHECK_ERROR(error);

	collect3 = pg_collect_new("collect3", &error);
	CHECK_ERROR(error);

	collect4 = pg_collect_new("collect4", &error);
	CHECK_ERROR(error);

	pg_brick_link(collect1, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(collect2, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(brick, collect3, &error);
	CHECK_ERROR(error);
	pg_brick_link(brick, collect4, &error);
	CHECK_ERROR(error);

	/* we will first exercise the learning abilities of the switch by
	 * a packets bursts with a given source address
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "10:10:10:10:10:10", "AA:AA:AA:AA:AA:AA");
	}
	/* west port 1 == collect 2 */
	pg_brick_burst_to_east(brick, 1, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect1 */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* collect3 */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect4 */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect4, &error) == 0);
	CHECK_ERROR(error);

	/* mac address "10:10:10:10:10:10" is now associated with west port 1
	 * brick which is collect2.
	 * We will now unlink and relink collect 2 to exercise the switch
	 * forgetting of the mac addresses associated with a given port.
	 */

	pg_brick_unlink(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_link(collect2, brick, &error);
	CHECK_ERROR(error);

	/* now we send a packet burst to "10:10:10:10:10:10"" and make sure
	 * that the switch unlearned it during the unlink by checking that the
	 * burst is forwarded to all non source ports.
	 */

	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "44:44:44:44:44:44", "10:10:10:10:10:10");
	}

	/* east port 1 == collect 4 */
	pg_brick_burst_to_west(brick, 1, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* check the burst has been forwarded to all port excepted the source
	 * one
	 */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	result_pkts = pg_brick_west_burst_get(collect4, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* potentially badly addressed packets */
	result_pkts = pg_brick_west_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_west_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_east_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = pg_brick_east_burst_get(collect4, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	pg_brick_unlink(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect3, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect4, &error);
	CHECK_ERROR(error);

	pg_brick_decref(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect3, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect4, &error);
	CHECK_ERROR(error);
	pg_brick_decref(brick, &error);

	CHECK_ERROR(error);
}

/* due to the multicast bit the multicast test also cover the broadcast case */
static void test_switch_multicast_destination(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick, *collect1, *collect2, *collect3;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = pg_switch_new("switch", 4, 4, PG_DEFAULT_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	collect1 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	collect2 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	collect3 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	pg_brick_link(collect1, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(collect2, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(brick, collect3, &error);
	CHECK_ERROR(error);

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "AA:AA:AA:AA:AA:AA", "11:11:11:11:11:11");
	}

	/* Send the packet through the port 0 of  switch it should been
	 * forwarded on all port because of the learning mode.
	 */
	pg_brick_burst_to_east(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);

	/* Now send responses packets (source and dest mac address swapped).
	 * The switch should forward to only one port
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "11:11:11:11:11:11", "AA:AA:AA:AA:AA:AA");
	}
	/* through port 0 of east side of the switch */
	pg_brick_burst_to_west(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet hould not have been forwarded to collect2 */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* and collect1 */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);

	/* Now the conversation can continue */
	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "AA:AA:AA:AA:AA:AA", "11:11:11:11:11:11");
	}

	pg_brick_burst_to_east(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the chain */
	pg_brick_unlink(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect3, &error);
	CHECK_ERROR(error);

	pg_brick_decref(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect3, &error);
	CHECK_ERROR(error);
	pg_brick_decref(brick, &error);

	CHECK_ERROR(error);
}
/* due to the multicast bit the multicast test also cover the broadcast case */
static void test_switch_multicast_both(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick, *collect1, *collect2, *collect3;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = pg_switch_new("switch", 4, 4, PG_DEFAULT_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	collect1 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	collect2 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	collect3 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	pg_brick_link(collect1, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(collect2, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(brick, collect3, &error);
	CHECK_ERROR(error);

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "33:33:33:33:33:33", "11:11:11:11:11:11");
	}

	/* Send the packet through the port 0 of  switch it should been
	 * forwarded on all port because of the learning mode.
	 */
	pg_brick_burst_to_east(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);

	/* Now send responses packets (source and dest mac address swapped).
	 * The switch should flood
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "11:11:11:11:11:11", "33:33:33:33:33:33");
	}
	/* through port 0 of east side of the switch */
	pg_brick_burst_to_west(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect1 */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);

	/* Now the conversation can continue */
	for (i = 0; i < NB_PKTS; i++) {
		pg_set_mac_addrs(pkts[i],
				 "33:33:33:33:33:33", "11:11:11:11:11:11");
	}

	pg_brick_burst_to_east(brick, 0, pkts, pg_mask_firsts(NB_PKTS),
			       &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	g_assert(pg_brick_reset(collect1, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_brick_reset(collect3, &error) == 0);
	CHECK_ERROR(error);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the chain */
	pg_brick_unlink(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect3, &error);
	CHECK_ERROR(error);

	pg_brick_decref(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect3, &error);
	CHECK_ERROR(error);
	pg_brick_decref(brick, &error);
	CHECK_ERROR(error);
}

static void test_switch_filtered(void)
{
#define FILTERED_COUNT 16
	struct pg_error *error = NULL;
	struct pg_brick *brick, *collect1, *collect2, *collect3;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = pg_switch_new("switch", 4, 4, PG_WEST_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	collect1 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	collect2 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	collect3 = pg_collect_new("collect", &error);
	CHECK_ERROR(error);

	pg_brick_link(collect1, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(collect2, brick, &error);
	CHECK_ERROR(error);
	pg_brick_link(brick, collect3, &error);
	CHECK_ERROR(error);

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < FILTERED_COUNT; i++) {
		char *filtered = g_strdup_printf("01:80:C2:00:00:%02X",
						 (uint8_t) i);

		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i], "33:33:33:33:33:33", filtered);
		g_free(filtered);
	}

	/* Send the packet through the port 0 of  switch it should been
	 * forwarded on all port because of the learning mode.
	 */
	pg_brick_burst_to_east(brick, 0, pkts,
			       pg_mask_firsts(FILTERED_COUNT), &error);
	CHECK_ERROR(error);

	/* the switch should not do source forward */
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	result_pkts = pg_brick_east_burst_get(collect2, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	result_pkts = pg_brick_west_burst_get(collect3, &pkts_mask, &error);
	CHECK_ERROR(error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	for (i = 0; i < FILTERED_COUNT; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the chain */
	pg_brick_unlink(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_unlink(collect3, &error);
	CHECK_ERROR(error);

	pg_brick_decref(collect1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect3, &error);
	CHECK_ERROR(error);
	pg_brick_decref(brick, &error);

	CHECK_ERROR(error);

#undef FILTERED_COUNT
}

static void test_switch_perf_learn(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick, *nops[TEST_PORTS];
	struct rte_mbuf *pkts[TEST_PKTS];
	uint64_t begin, delta;
	uint16_t i;
	uint32_t j;

	brick = pg_switch_new("switch", TEST_PORTS / 2, TEST_PORTS / 2,
			      PG_DEFAULT_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	for (i = 0; i < TEST_PORTS; i++) {
		nops[i] = pg_nop_new("nop", &error);
		CHECK_ERROR(error);
	}

	for (i = 0; i < TEST_PORTS / 2; i++) {
		pg_brick_link(nops[i], brick, &error);
		CHECK_ERROR(error);
	}

	for (i = 0; i < TEST_PORTS / 2; i++) {
		pg_brick_link(brick, nops[i + TEST_PORTS / 2], &error);
		CHECK_ERROR(error);
	}

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < TEST_PKTS; i++) {
		uint8_t major, minor;

		major = (i >> 8) & 0xFF;
		minor = i & 0xFF;
		char *mac = g_strdup_printf("AA:AA:AA:AA:%02X:%02X",
					    major, minor);

		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i], mac, "AA:AA:AA:AA:AA:AA");
		g_free(mac);
	}

	/* test the learning speed */
	begin = g_get_real_time();
	for (j = 0; j < TEST_PKTS_COUNT; j += PKTS_BURST) {
		struct rte_mbuf **to_burst = &pkts[j % TEST_MOD];

		pg_brick_burst_to_east(brick, 0,
				       to_burst,
				       pg_mask_firsts(PKTS_BURST), &error);
		CHECK_ERROR(error);
	}
	delta = g_get_real_time() - begin;

	printf("Switching %"PRIi64" packets per second in learning mode. ",
	       (uint64_t) (TEST_PKTS_COUNT * ((1000000 * 1.0) / delta)));

	for (i = 0; i < TEST_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);


	for (i = 0; i < TEST_PORTS / 2; i++) {
		pg_brick_unlink(nops[i + TEST_PORTS / 2], &error);
		CHECK_ERROR(error);
	}
	for (i = 0; i < TEST_PORTS / 2; i++) {
		pg_brick_unlink(nops[i], &error);
		CHECK_ERROR(error);
	}

	for (i = 0; i < TEST_PORTS; i++) {
		pg_brick_decref(nops[i], &error);
		CHECK_ERROR(error);
	}

	pg_brick_decref(brick, &error);
	CHECK_ERROR(error);
}

static void test_switch_perf_switch(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick, *nops[TEST_PORTS];
	struct rte_mbuf *pkts[TEST_PKTS];
	uint64_t begin, delta;
	struct rte_mbuf **to_burst;
	uint16_t i;
	uint32_t j;

	brick = pg_switch_new("switch", TEST_PORTS / 2, TEST_PORTS / 2,
			      PG_DEFAULT_SIDE, &error);
	g_assert(brick);
	CHECK_ERROR(error);

	for (i = 0; i < TEST_PORTS; i++) {
		nops[i] = pg_nop_new("nop", &error);
		CHECK_ERROR(error);
	}

	for (i = 0; i < SIDE_PORTS; i++) {
		pg_brick_link(nops[i], brick, &error);
		CHECK_ERROR(error);
	}

	for (i = 0; i < SIDE_PORTS; i++) {
		pg_brick_link(brick, nops[i + TEST_PORTS / 2], &error);
		CHECK_ERROR(error);
	}

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < TEST_PKTS; i++) {
		uint8_t major, minor;

		major = (i >> 8) & 0xFF;
		minor = i & 0xFF;
		char *mac = g_strdup_printf("AA:AA:AA:AA:%02X:%02X",
					    major, minor);

		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i], mac, "AA:AA:AA:AA:AA:AA");
		g_free(mac);
	}

	/* mac the switch learn the sources mac addresses */
	for (j = 0; j < TEST_PKTS; j += 2) {
		to_burst = &pkts[j];
		pg_brick_burst_to_east(brick, j % SIDE_PORTS, to_burst,
				       pg_mask_firsts(1), &error);
		CHECK_ERROR(error);

		to_burst = &pkts[j + 1];
		pg_brick_burst_to_west(brick, j % SIDE_PORTS, to_burst,
				       pg_mask_firsts(1), &error);
		CHECK_ERROR(error);
	}

	for (i = 0; i < TEST_PKTS; i++) {
		uint8_t major, minor;

		major = (i >> 8) & 0xFF;
		minor = i & 0xFF;
		char *mac = g_strdup_printf("AA:AA:AA:AA:%02X:%02X",
					    major, minor);

		char *reverse = g_strdup_printf("AA:AA:AA:AA:%02X:%02X",
						minor, major);


		pg_set_mac_addrs(pkts[i], reverse, mac);
		g_free(mac);
		g_free(reverse);
	}

	begin = g_get_real_time();
	for (j = 0; j < TEST_PKTS_COUNT; j += PKTS_BURST *  2) {
		to_burst = &pkts[(j % TEST_MOD)];
		pg_brick_burst_to_east(brick, SIDE_PORTS - (j % SIDE_PORTS) - 1,
				       to_burst,
				       pg_mask_firsts(PKTS_BURST), &error);
		CHECK_ERROR(error);
		to_burst = &pkts[(j + PKTS_BURST) % TEST_MOD];
		pg_brick_burst_to_east(brick, SIDE_PORTS - (j % SIDE_PORTS) - 1,
				       to_burst,
				       pg_mask_firsts(PKTS_BURST), &error);
		CHECK_ERROR(error);
	}
	delta = g_get_real_time() - begin;

	printf("Switching %"PRIi64" packets per second in switching mode. ",
	       (uint64_t) (TEST_PKTS_COUNT * ((1000000 * 1.0) / delta)));

	for (i = 0; i < TEST_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);


	for (i = 0; i < TEST_PORTS / 2; i++) {
		pg_brick_unlink(nops[i + TEST_PORTS / 2], &error);
		CHECK_ERROR(error);
	}
	for (i = 0; i < TEST_PORTS / 2; i++) {
		pg_brick_unlink(nops[i], &error);
		CHECK_ERROR(error);
	}

	for (i = 0; i < TEST_PORTS; i++) {
		pg_brick_decref(nops[i], &error);
		CHECK_ERROR(error);
	}

	pg_brick_decref(brick, &error);
	CHECK_ERROR(error);

#undef TEST_PORTS
#undef TEST_PKTS
#undef TEST_PKTS_COUNT
}

static void test_switch(void)
{
	mbuf_pool = pg_get_mempool();
	pg_test_add_func("/switch/lifecycle", test_switch_lifecycle);
	pg_test_add_func("/switch/learn", test_switch_learn);
	pg_test_add_func("/switch/switching", test_switch_switching);
	pg_test_add_func("/switch/unlink", test_switch_unlink);
	pg_test_add_func("/switch/multicast/destination",
			test_switch_multicast_destination);
	pg_test_add_func("/switch/multicast/both", test_switch_multicast_both);
	pg_test_add_func("/switch/filtered", test_switch_filtered);
	pg_test_add_func("/switch/perf/learn", test_switch_perf_learn);
	pg_test_add_func("/switch/perf/switch", test_switch_perf_switch);
}

int main(int argc, char **argv)
{
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	g_assert(pg_start(argc, argv) >= 0);

	test_switch();
	int r = g_test_run();

	pg_stop();
	return r;
}
