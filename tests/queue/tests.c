/* Copyright 2014 Nodalink EURL

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
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/mac.h"
#include "collect.h"
#include <packetgraph/packetgraph.h>

#define CHECK_ERROR(error) do {                 \
               if (error)                       \
		       pg_error_print(error);   \
               g_assert(!error);                \
       } while (0)

/* copied struct for testing purpose */
struct pg_queue_state {
        struct pg_brick brick;
        enum pg_side output;
        uint32_t rx_max_size;
        GAsyncQueue *rx;
        struct pg_queue_state *friend;
};

static void test_queue_lifecycle(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick;
	struct pg_queue_state *state;

	brick = pg_queue_new("test_queue", 0, &error);
	g_assert(brick);
	CHECK_ERROR(error);
	state = pg_brick_get_state(brick, struct pg_queue_state);
	g_assert(state->rx_max_size == 10);
	pg_brick_destroy(brick);
	brick = pg_queue_new("test_queue", 1, &error);
	g_assert(brick);
	CHECK_ERROR(error);
	state = pg_brick_get_state(brick, struct pg_queue_state);
	g_assert(state->rx_max_size == 1);

	pg_brick_destroy(brick);
	CHECK_ERROR(error);
}

static void test_queue_friend(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *q1, *q2, *q3;

	q1 = pg_queue_new("q1", 1, &error);
	CHECK_ERROR(error);
	q2 = pg_queue_new("q2", 1, &error);
	CHECK_ERROR(error);
	q3 = pg_queue_new("q3", 1, &error);
	CHECK_ERROR(error);

	/* classic scenario */
	g_assert(!pg_queue_get_friend(q1));
	g_assert(!pg_queue_get_friend(q2));
	g_assert(!pg_queue_friend(q1, q2, &error));
	CHECK_ERROR(error);
	g_assert(pg_queue_get_friend(q1) == q2);
	g_assert(pg_queue_are_friend(q1, q2));
	g_assert(pg_queue_get_friend(q2) == q1);
	g_assert(pg_queue_are_friend(q2, q1));
	pg_queue_unfriend(q1);
	g_assert(!pg_queue_get_friend(q1));
	g_assert(!pg_queue_are_friend(q1, q2));
	g_assert(!pg_queue_get_friend(q2));
	g_assert(!pg_queue_are_friend(q2, q1));

	/* same but unfriend with second brick */
	g_assert(!pg_queue_friend(q1, q2, &error));
	CHECK_ERROR(error);
	g_assert(pg_queue_are_friend(q1, q2));
	g_assert(pg_queue_are_friend(q2, q1));
	pg_queue_unfriend(q2);
	g_assert(!pg_queue_get_friend(q1));
	g_assert(!pg_queue_get_friend(q2));

	/* friend with itself */
	g_assert(!pg_queue_friend(q1, q1, &error));
	CHECK_ERROR(error);
	g_assert(pg_queue_get_friend(q1) == q1);
	g_assert(pg_queue_are_friend(q1, q1));
	pg_queue_unfriend(q1);
	g_assert(!pg_queue_get_friend(q1));

	/* several unfriend ok ? (test should die if not) */
	pg_queue_unfriend(q1);
	pg_queue_unfriend(q1);
	pg_queue_unfriend(q1);

	/* error if already friend */
	g_assert(!pg_queue_friend(q1, q2, &error));
	CHECK_ERROR(error);
	g_assert(pg_queue_friend(q1, q2, &error) == -1);
	g_assert(pg_error_is_set(&error));
	pg_error_free(error);
	error = NULL;
	g_assert(pg_queue_friend(q2, q1, &error) == -1);
	g_assert(pg_error_is_set(&error));
	pg_error_free(error);
	error = NULL;
	g_assert(pg_queue_friend(q1, q3, &error) == -1);
	g_assert(pg_error_is_set(&error));
	pg_error_free(error);
	error = NULL;
	g_assert(pg_queue_friend(q2, q3, &error) == -1);
	g_assert(pg_error_is_set(&error));
	pg_error_free(error);
	error = NULL;
	g_assert(pg_queue_friend(q3, q1, &error) == -1);
	g_assert(pg_error_is_set(&error));
	pg_error_free(error);
	error = NULL;
	g_assert(pg_queue_friend(q3, q2, &error) == -1);
	g_assert(pg_error_is_set(&error));
	pg_error_free(error);
	error = NULL;

	/* check that death break friendship */
	pg_brick_decref(q1, &error);
	CHECK_ERROR(error);
	g_assert(!pg_queue_get_friend(q2));

	/* ... and can be friend again */
	g_assert(!pg_queue_friend(q2, q3, &error));
	g_assert(pg_queue_are_friend(q2, q3));
	g_assert(pg_queue_are_friend(q3, q2));

	pg_brick_decref(q2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(q3, &error);
	CHECK_ERROR(error);
}

static void test_queue_burst(void)
{
#	define NB_PKTS 64
	struct pg_error *error = NULL;
	struct pg_brick *queue1, *queue2, *collect;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[NB_PKTS];
	uint64_t pkts_mask, i, j;
	uint16_t count = 0;
	struct rte_mempool *mbuf_pool = pg_get_mempool();

	/**
	 * Burst packets in queue1 to get them in collect
	 * [queue1] ~ [queue2]----[collect]
	 */
	queue1 = pg_queue_new("q1", 10, &error);
	CHECK_ERROR(error);
	queue2 = pg_queue_new("q2", 10, &error);
	CHECK_ERROR(error);
	collect = pg_collect_new("collect", 1, 1, &error);
	CHECK_ERROR(error);

	pg_brick_link(queue2, collect, &error);
	CHECK_ERROR(error);
	g_assert(!pg_queue_friend(queue1, queue2, &error));
	CHECK_ERROR(error);

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "F0:F1:F2:F3:F4:F5",
				 "E0:E1:E2:E3:E4:E5");
	}

	for (j = 0; j < 100; j++) {
		for (i = 0; i < NB_PKTS; i++)
			pkts[i]->udata64 = i * j;
		pg_brick_burst_to_east(queue1, 0, pkts, pg_mask_firsts(NB_PKTS),
				       &error);
		CHECK_ERROR(error);

		pg_brick_poll(queue2, &count, &error);
		CHECK_ERROR(error);
		g_assert(count == NB_PKTS);

		result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
						      &error);
		CHECK_ERROR(error);
		g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
		for (i = 0; i < NB_PKTS; i++) {
			g_assert(result_pkts[i]);
			g_assert(result_pkts[i]->udata64 == i * j);
		}
		g_assert(pg_brick_reset(collect, &error) == 0);
		CHECK_ERROR(error);
	}

	/* clean */
	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);
	pg_brick_decref(queue1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(queue2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect, &error);
	CHECK_ERROR(error);
#	undef NB_PKTS
}

static void test_queue_limit(void)
{
#	define NB_PKTS 64
	struct pg_error *error = NULL;
	struct pg_brick *queue1, *queue2, *collect;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[NB_PKTS];
	uint64_t pkts_mask, i, j;
	uint16_t count = 0;
	struct rte_mempool *mbuf_pool = pg_get_mempool();

	/**
	 * Burst packets in queue1 to get them in collect
	 * [queue1] ~ [queue2]----[collect]
	 */
	queue1 = pg_queue_new("q1", 10, &error);
	CHECK_ERROR(error);
	queue2 = pg_queue_new("q2", 10, &error);
	CHECK_ERROR(error);
	collect = pg_collect_new("collect", 1, 1, &error);
	CHECK_ERROR(error);

	pg_brick_link(queue2, collect, &error);
	CHECK_ERROR(error);
	g_assert(!pg_queue_friend(queue1, queue2, &error));
	CHECK_ERROR(error);

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "F0:F1:F2:F3:F4:F5",
				 "E0:E1:E2:E3:E4:E5");
	}

	/* let's burst on queue limit */
	g_assert(pg_queue_pressure(queue1) == 0);
	g_assert(pg_queue_pressure(queue2) == 0);
	for (j = 0; j < 10; j++) {
		pg_brick_burst_to_east(queue1, 0, pkts, pg_mask_firsts(NB_PKTS),
				       &error);
		CHECK_ERROR(error);
	}
	g_assert(pg_queue_pressure(queue1) == 255);
	g_assert(pg_queue_pressure(queue2) == 0);

	for (j = 0; j < 10; j++) {
		pg_brick_poll(queue2, &count, &error);
		CHECK_ERROR(error);
		g_assert(count == NB_PKTS);

		result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
						      &error);
		CHECK_ERROR(error);
		g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
		for (i = 0; i < NB_PKTS; i++) {
			g_assert(result_pkts[i]);
			g_assert(result_pkts[i]->udata64 == i);
		}
		g_assert(pg_brick_reset(collect, &error) == 0);
	}
	g_assert(pg_queue_pressure(queue1) == 0);
	g_assert(pg_queue_pressure(queue2) == 0);
	pg_brick_poll(queue2, &count, &error);
	CHECK_ERROR(error);
	g_assert(count == 0);
	result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
					      &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(0));
	g_assert(pg_queue_pressure(queue1) == 0);
	g_assert(pg_queue_pressure(queue2) == 0);

	/* let's burst over queue limit */
	for (j = 0; j < 100; j++) {
		pg_brick_burst_to_east(queue1, 0, pkts, pg_mask_firsts(NB_PKTS),
				       &error);
		CHECK_ERROR(error);
	}
	g_assert(pg_queue_pressure(queue1) == 255);
	g_assert(pg_queue_pressure(queue2) == 0);

	for (j = 0; j < 10; j++) {
		pg_brick_poll(queue2, &count, &error);
		CHECK_ERROR(error);
		g_assert(count == NB_PKTS);

		result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
						      &error);
		CHECK_ERROR(error);
		g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
		for (i = 0; i < NB_PKTS; i++) {
			g_assert(result_pkts[i]);
			g_assert(result_pkts[i]->udata64 == i);
		}
		g_assert(pg_brick_reset(collect, &error) == 0);
	}
	g_assert(pg_queue_pressure(queue1) == 0);
	g_assert(pg_queue_pressure(queue2) == 0);
	pg_brick_poll(queue2, &count, &error);
	CHECK_ERROR(error);
	g_assert(count == 0);
	result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
					      &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(0));
	g_assert(pg_queue_pressure(queue1) == 0);
	g_assert(pg_queue_pressure(queue2) == 0);

	/* same but poll _after_ being friend */
	pg_queue_unfriend(queue1);
	g_assert(!pg_queue_get_friend(queue1));
	g_assert(!pg_queue_get_friend(queue2));

	for (j = 0; j < 100; j++) {
		pg_brick_burst_to_east(queue1, 0, pkts, pg_mask_firsts(NB_PKTS),
				       &error);
		CHECK_ERROR(error);
	}
	g_assert(pg_queue_pressure(queue1) == 255);
	g_assert(pg_queue_pressure(queue2) == 0);

	g_assert(!pg_brick_poll(queue2, &count, &error));
	CHECK_ERROR(error);
	g_assert(count == 0);
	result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
					      &error);
	g_assert(pkts_mask == pg_mask_firsts(0));
	g_assert(pg_queue_pressure(queue1) == 255);
	g_assert(pg_queue_pressure(queue2) == 0);

	g_assert(!pg_queue_friend(queue1, queue2, &error));
	CHECK_ERROR(error);

	g_assert(pg_queue_are_friend(queue1, queue2));
	g_assert(pg_queue_are_friend(queue2, queue1));
	g_assert(pg_queue_pressure(queue1) == 255);
	g_assert(pg_queue_pressure(queue2) == 0);
	for (j = 0; j < 10; j++) {
		pg_brick_poll(queue2, &count, &error);
		CHECK_ERROR(error);
		g_assert(count == NB_PKTS);

		result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
						      &error);
		CHECK_ERROR(error);
		g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
		for (i = 0; i < NB_PKTS; i++) {
			g_assert(result_pkts[i]);
			g_assert(result_pkts[i]->udata64 == i);
		}
		g_assert(pg_brick_reset(collect, &error) == 0);
	}
	g_assert(pg_queue_pressure(queue1) == 0);
	g_assert(pg_queue_pressure(queue2) == 0);
	pg_brick_poll(queue2, &count, &error);
	CHECK_ERROR(error);
	g_assert(count == 0);
	result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
					      &error);
	CHECK_ERROR(error);
	g_assert(pkts_mask == pg_mask_firsts(0));
	g_assert(pg_queue_pressure(queue1) == 0);
	g_assert(pg_queue_pressure(queue2) == 0);

	/* clean */
	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);
	pg_brick_decref(queue1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(queue2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect, &error);
	CHECK_ERROR(error);
#	undef NB_PKTS
}

static void test_queue_reset(void)
{
#	define NB_PKTS 64
	struct pg_error *error = NULL;
	struct pg_brick *queue1, *queue2, *collect;
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[NB_PKTS];
	uint64_t pkts_mask, i, j;
	uint16_t count = 0;
	struct rte_mempool *mbuf_pool = pg_get_mempool();

	/**
	 * Burst packets in queue1 and test reset of queue1
	 * [queue1] ~ [queue2]----[collect]
	 */
	queue1 = pg_queue_new("q1", 10, &error);
	CHECK_ERROR(error);
	queue2 = pg_queue_new("q2", 10, &error);
	CHECK_ERROR(error);
	collect = pg_collect_new("collect", 1, 1, &error);
	CHECK_ERROR(error);

	pg_brick_link(queue2, collect, &error);
	CHECK_ERROR(error);
	g_assert(!pg_queue_friend(queue1, queue2, &error));
	CHECK_ERROR(error);

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "F0:F1:F2:F3:F4:F5",
				 "E0:E1:E2:E3:E4:E5");
	}

	for (j = 0; j < 100; j++) {
		for (i = 0; i < NB_PKTS; i++)
			pkts[i]->udata64 = i * j;
		/* burst and reset */
		pg_brick_burst_to_east(queue1, 0, pkts, pg_mask_firsts(NB_PKTS),
				       &error);
		CHECK_ERROR(error);
		g_assert(pg_queue_pressure(queue1) > 0);
		g_assert(pg_brick_reset(queue1, &error) == 0);
		g_assert(pg_queue_get_friend(queue1) == NULL);
		g_assert(pg_queue_get_friend(queue2) == NULL);
		g_assert(pg_queue_pressure(queue1) == 0);
		g_assert(pg_queue_pressure(queue2) == 0);
		pg_brick_poll(queue2, &count, &error);
		g_assert(!error);
		g_assert(count == 0);

		/* refriend and burst ok */
		g_assert(!pg_queue_friend(queue1, queue2, &error));
		g_assert(!error);
		g_assert(pg_queue_are_friend(queue1, queue2));
		g_assert(!error);

		pg_brick_burst_to_east(queue1, 0, pkts, pg_mask_firsts(NB_PKTS),
				       &error);
		CHECK_ERROR(error);
		g_assert(pg_queue_pressure(queue1) > 0);
		pg_brick_poll(queue2, &count, &error);
		g_assert(count == NB_PKTS);
		result_pkts = pg_brick_west_burst_get(collect, &pkts_mask,
						      &error);
		CHECK_ERROR(error);
		g_assert(pkts_mask == pg_mask_firsts(NB_PKTS));
		for (i = 0; i < NB_PKTS; i++) {
			g_assert(result_pkts[i]);
			g_assert(result_pkts[i]->udata64 == i * j);
		}
		g_assert(pg_brick_reset(collect, &error) == 0);
		CHECK_ERROR(error);
	}

	/* clean */
	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);
	pg_brick_decref(queue1, &error);
	CHECK_ERROR(error);
	pg_brick_decref(queue2, &error);
	CHECK_ERROR(error);
	pg_brick_decref(collect, &error);
	CHECK_ERROR(error);
#	undef NB_PKTS
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

	g_test_add_func("/queue/lifecycle", test_queue_lifecycle);
	g_test_add_func("/queue/friend", test_queue_friend);
	g_test_add_func("/queue/burst", test_queue_burst);
	g_test_add_func("/queue/limit", test_queue_limit);
	g_test_add_func("/queue/reset", test_queue_reset);
	r = g_test_run();

	pg_stop();
	return r;
}
