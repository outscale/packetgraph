/* Copyright 2016 Outscale SAS
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
#include <glib/gprintf.h>
#include <string.h>
#include <rte_config.h>
#include <rte_ether.h>
#include <packetgraph/packetgraph.h>
#include "tests.h"
#include "packets.h"
#include "brick-int.h"
#include "utils/bitmask.h"
#include "utils/mempool.h"
#include "utils/mac.h"

static void test_graph_lifecycle(void)
{
	struct pg_graph *g;
	struct pg_error *error = NULL;
	struct pg_brick *nop_1, *nop_2, *que_3;

	/* simplest allocation */
	g = pg_graph_new("g1", NULL, &error);
	g_assert(g && !error);
	g_assert(!g_strcmp0(pg_graph_name(g), "g1"));
	g_assert(pg_graph_count(g) == 0);
	pg_graph_destroy(g);

	/* allocation errors */
	nop_1 = pg_nop_new("nop", &error);
	g_assert(!error);
	nop_2 = pg_nop_new("nop", &error);
	g_assert(!error);
	g_assert(!pg_brick_link(nop_1, nop_2, &error));
	g_assert(!error);
	g = pg_graph_new("gerror", nop_1, &error);
	g_assert(g == NULL);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	pg_brick_destroy(nop_1);
	pg_brick_destroy(nop_2);

	/* allocation with propagation (only one brick) */
	nop_1 = pg_nop_new("nop", &error);
	g_assert(nop_1 != NULL);
	g_assert(!error);
	g = pg_graph_new("noerror", nop_1, &error);
	g_assert(g != NULL);
	g_assert(!error);
	g_assert(pg_graph_count(g) == 1);
	pg_graph_destroy(g);

	/* no allocation and push bricks */
	g = pg_graph_new("g2", NULL, &error);
	g_assert(g && !error);

	for (int i = 0; i < 10; i++) {
		g_assert(!g_strcmp0(pg_graph_name(g), "g2"));
		g_assert(pg_graph_count(g) == 0);

		nop_1 = pg_nop_new("nop1", &error);
		nop_2 = pg_nop_new("nop2", &error);
		que_3 = pg_queue_new("que3", -1, &error);
		g_assert(!error);
		g_assert(!pg_graph_push(g, nop_1, &error));
		g_assert(!error);
		g_assert(pg_graph_count(g) == 1);
		g_assert(pg_graph_push(g, nop_1, &error) == -1);
		g_assert(error);
		pg_error_free(error);
		error = NULL;
		g_assert(pg_graph_count(g) == 1);
		g_assert(!pg_graph_push(g, nop_2, &error));
		g_assert(!error);
		g_assert(pg_graph_count(g) == 2);
		g_assert(pg_graph_push(g, nop_2, &error) == -1);
		g_assert(error);
		pg_error_free(error);
		error = NULL;
		g_assert(pg_graph_push(g, nop_1, &error) == -1);
		g_assert(error);
		pg_error_free(error);
		error = NULL;
		g_assert(pg_graph_count(g) == 2);
		g_assert(!pg_graph_push(g, que_3, &error));
		g_assert(!error);
		g_assert(pg_graph_count(g) == 3);
		g_assert(pg_graph_push(g, que_3, &error) == -1);
		g_assert(error);
		pg_error_free(error);
		error = NULL;
		g_assert(pg_graph_push(g, nop_2, &error) == -1);
		g_assert(error);
		pg_error_free(error);
		error = NULL;
		g_assert(pg_graph_push(g, nop_1, &error) == -1);
		g_assert(error);
		pg_error_free(error);
		error = NULL;
		g_assert(pg_graph_count(g) == 3);

		g_assert(pg_graph_get(g, "nop1") == nop_1);
		g_assert(pg_graph_get(g, "nop2") == nop_2);
		g_assert(pg_graph_get(g, "que3") == que_3);
		g_assert(pg_graph_get(g, "que32") == NULL);

		pg_graph_destroy(g);
		g = pg_graph_new("g2", NULL, &error);
		g_assert(g && !error);
		nop_1 = pg_nop_new("nop10", &error);
		nop_2 = pg_nop_new("nop20", &error);
		que_3 = pg_queue_new("que30", -1, &error);
		g_assert(!error);
		g_assert(!pg_graph_push(g, nop_1, &error));
		g_assert(!error);
		g_assert(!pg_graph_push(g, nop_2, &error));
		g_assert(!error);
		g_assert(!pg_graph_push(g, que_3, &error));
		g_assert(!error);
		g_assert(pg_graph_count(g) == 3);
		g_assert(pg_graph_get(g, "nop1") == NULL);
		g_assert(pg_graph_get(g, "nop2") == NULL);
		g_assert(pg_graph_get(g, "que3") == NULL);
		g_assert(pg_graph_get(g, "nop10") == nop_1);
		g_assert(pg_graph_get(g, "nop20") == nop_2);
		g_assert(pg_graph_get(g, "que30") == que_3);

		g_assert(pg_graph_pop(g, "nop1") == NULL);
		g_assert(pg_graph_pop(g, "nop2") == NULL);
		g_assert(pg_graph_pop(g, "que3") == NULL);
		g_assert(pg_graph_count(g) == 3);
		g_assert(pg_graph_pop(g, "nop10") == nop_1);
		g_assert(pg_graph_count(g) == 2);
		g_assert(pg_graph_pop(g, "nop20") == nop_2);
		g_assert(pg_graph_count(g) == 1);
		g_assert(pg_graph_pop(g, "que30") == que_3);
		g_assert(pg_graph_count(g) == 0);
		g_assert(!pg_graph_push(g, nop_1, &error));
		g_assert(!error);
		g_assert(!pg_graph_push(g, nop_2, &error));
		g_assert(!error);
		g_assert(!pg_graph_push(g, que_3, &error));
		g_assert(!error);
		g_assert(pg_graph_count(g) == 3);

		g_assert(pg_graph_brick_destroy(g, "LOL", &error) == -1);
		g_assert(error);
		pg_error_free(error);
		error = NULL;
		g_assert(!pg_graph_brick_destroy(g, "nop10", &error));
		g_assert(!error);
		g_assert(pg_graph_count(g) == 2);
		g_assert(pg_graph_get(g, "nop10") == NULL);
		g_assert(pg_graph_get(g, "nop20") == nop_2);
		g_assert(pg_graph_get(g, "que30") == que_3);
		g_assert(!pg_graph_brick_destroy(g, "que30", &error));
		g_assert(!error);
		g_assert(pg_graph_count(g) == 1);
		g_assert(pg_graph_get(g, "nop10") == NULL);
		g_assert(pg_graph_get(g, "nop20") == nop_2);
		g_assert(pg_graph_get(g, "que30") == NULL);
		g_assert(!pg_graph_brick_destroy(g, "nop20", &error));
		g_assert(!error);
		g_assert(pg_graph_count(g) == 0);
		g_assert(pg_graph_get(g, "nop10") == NULL);
		g_assert(pg_graph_get(g, "nop20") == NULL);
		g_assert(pg_graph_get(g, "que30") == NULL);
	}
}

static void test_graph_explore(void)
{
	/* test exploration with this graph:
	 * (queue2 is friend with queue3)
	 *
	 *           ___________                  ___________
	 *           |         |                  |         |
	 * [nop0]----+         +--[nop2]--[nop4]--+         +--[queue1]
	 *           |         |                  |         |
	 * [nop1]----+ switch0 |                  | switch1 |
	 *           |         |                  |         |
	 * [queue0]--+         +--[nop3]----------+         +--[nop5]
	 *           |_________|                  |_________|
	 *
	 * [queue1] ~ [queue2]--[nop6]
	 *
	 */
	struct pg_graph *g1, *g2;
	struct pg_error *error = NULL;
	struct pg_brick *nop[7], *queue[3], *sw[2];
	gchar *tmp;

	for (int i = 0; i < 7; i++) {
		tmp = g_strdup_printf("nop%i", i);
		nop[i] = pg_nop_new(tmp, &error);
		g_free(tmp);
	}
	for (int i = 0; i < 3; i++) {
		tmp = g_strdup_printf("queue%i", i);
		queue[i] = pg_queue_new(tmp, 0, &error);
		g_free(tmp);
	}
	for (int i = 0; i < 2; i++) {
		tmp = g_strdup_printf("switch%i", i);
		sw[i] = pg_switch_new(tmp, 3, 3, PG_WEST_SIDE, &error);
		g_free(tmp);
	}

	g_assert(!pg_brick_chained_links(&error, nop[0], sw[0], nop[2], nop[4],
					 sw[1], queue[1]));
	g_assert(!error);
	g_assert(!pg_brick_chained_links(&error, queue[0], sw[0], nop[3],
					 sw[1], nop[5]));
	g_assert(!error);
	g_assert(!pg_brick_link(nop[1], sw[0], &error));
	g_assert(!error);
	g_assert(!pg_queue_friend(queue[1], queue[2], &error));
	g_assert(!error);
	g_assert(!pg_brick_link(queue[2], nop[6], &error));
	g_assert(!error);

	g1 = pg_graph_new("graph1", nop[0], &error);
	g_assert(g1 && !error);
	g_assert(pg_graph_count(g1) == 10);
	g2 = pg_graph_new("graph2", queue[2], &error);
	g_assert(g2 && !error);
	g_assert(pg_graph_count(g2) == 2);
	for (int i = 0; i < 6; i++) {
		tmp = g_strdup_printf("nop%i", i);
		g_assert(pg_graph_get(g1, tmp) == nop[i]);
		g_assert(pg_graph_get(g2, tmp) == NULL);
		g_free(tmp);
	}
	g_assert(pg_graph_get(g1, "nop6") == NULL);
	g_assert(pg_graph_get(g2, "nop6") == nop[6]);
	g_assert(pg_graph_get(g1, "switch0") == sw[0]);
	g_assert(pg_graph_get(g2, "switch0") == NULL);
	g_assert(pg_graph_get(g1, "switch1") == sw[1]);
	g_assert(pg_graph_get(g2, "switch1") == NULL);
	g_assert(pg_graph_get(g1, "queue0") == queue[0]);
	g_assert(pg_graph_get(g2, "queue0") == NULL);
	g_assert(pg_graph_get(g1, "queue1") == queue[1]);
	g_assert(pg_graph_get(g2, "queue1") == NULL);
	g_assert(pg_graph_get(g1, "queue2") == NULL);
	g_assert(pg_graph_get(g2, "queue2") == queue[2]);

	/* break links [nop2]--[nop4] and [nop3]--[switch1]
	 * explore graph1 on the left size, explore graph2 on the right side
	 */
	g_assert(!pg_brick_unlink_edge(nop[2], nop[4], &error));
	g_assert(!pg_brick_unlink_edge(nop[3], sw[1], &error));

	g_assert(!pg_graph_explore(g1, "nop0", &error));
	g_assert(pg_graph_explore(g2, "switch1", &error) == -1);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	g_assert(!pg_graph_explore_ptr(g2, sw[1], &error));
	g_assert(pg_graph_count(g1) == 6);
	g_assert(pg_graph_count(g2) == 4);
	g_assert(pg_graph_get(g1, "nop0") == nop[0]);
	g_assert(pg_graph_get(g2, "nop0") == NULL);
	g_assert(pg_graph_get(g1, "nop1") == nop[1]);
	g_assert(pg_graph_get(g2, "nop1") == NULL);
	g_assert(pg_graph_get(g1, "nop2") == nop[2]);
	g_assert(pg_graph_get(g2, "nop2") == NULL);
	g_assert(pg_graph_get(g1, "nop3") == nop[3]);
	g_assert(pg_graph_get(g2, "nop3") == NULL);
	g_assert(pg_graph_get(g1, "queue0") == queue[0]);
	g_assert(pg_graph_get(g2, "queue0") == NULL);
	g_assert(pg_graph_get(g1, "nop4") == NULL);
	g_assert(pg_graph_get(g2, "nop4") == nop[4]);
	g_assert(pg_graph_get(g1, "nop5") == NULL);
	g_assert(pg_graph_get(g2, "nop5") == nop[5]);
	g_assert(pg_graph_get(g1, "nop6") == NULL);
	g_assert(pg_graph_get(g2, "nop6") == NULL);
	g_assert(pg_graph_get(g1, "queue1") == NULL);
	g_assert(pg_graph_get(g2, "queue1") == queue[1]);
	g_assert(pg_graph_get(g1, "queue2") == NULL);
	g_assert(pg_graph_get(g2, "queue2") == NULL);
	g_assert(pg_graph_get(g1, "switch0") == sw[0]);
	g_assert(pg_graph_get(g2, "switch0") == NULL);
	g_assert(pg_graph_get(g1, "switch1") == NULL);
	g_assert(pg_graph_get(g2, "switch1") == sw[1]);

	/* relink like before */
	g_assert(!pg_brick_link(nop[2], nop[4], &error));
	g_assert(!error);
	g_assert(!pg_brick_link(nop[3], sw[1], &error));
	g_assert(!error);
	g_assert(pg_graph_explore(g1, "switch1", &error) == -1);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	g_assert(!pg_graph_explore_ptr(g1, sw[1], &error));
	g_assert(!error);
	g_assert(pg_graph_explore(g2, "queue2", &error) == -1);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	g_assert(!pg_graph_explore_ptr(g2, queue[2], &error));
	g_assert(pg_graph_count(g1) == 10);
	g_assert(pg_graph_count(g2) == 2);
	for (int i = 0; i < 6; i++) {
		tmp = g_strdup_printf("nop%i", i);
		g_assert(pg_graph_get(g1, tmp) == nop[i]);
		g_assert(pg_graph_get(g2, tmp) == NULL);
		g_free(tmp);
	}
	g_assert(pg_graph_get(g1, "nop6") == NULL);
	g_assert(pg_graph_get(g2, "nop6") == nop[6]);
	g_assert(pg_graph_get(g1, "switch0") == sw[0]);
	g_assert(pg_graph_get(g2, "switch0") == NULL);
	g_assert(pg_graph_get(g1, "switch1") == sw[1]);
	g_assert(pg_graph_get(g2, "switch1") == NULL);
	g_assert(pg_graph_get(g1, "queue0") == queue[0]);
	g_assert(pg_graph_get(g2, "queue0") == NULL);
	g_assert(pg_graph_get(g1, "queue1") == queue[1]);
	g_assert(pg_graph_get(g2, "queue1") == NULL);
	g_assert(pg_graph_get(g1, "queue2") == NULL);
	g_assert(pg_graph_get(g2, "queue2") == queue[2]);

	pg_graph_destroy(g1);
	pg_graph_destroy(g2);
}

static void test_graph_sanity(void)
{
	/* Let's have a simple graph:
	 *
	 * [nop0]--[nop1]--[nop2]--[nop3]
	 *
	 * and break it in the middle, should not be sane anymore
	 */
	struct pg_graph *g;
	struct pg_error *error = NULL;
	struct pg_brick *nop[4];
	gchar *tmp;

	for (int i = 0; i < 4; i++) {
		tmp = g_strdup_printf("nop%i", i);
		nop[i] = pg_nop_new(tmp, &error);
		g_free(tmp);
	}

	g_assert(!pg_brick_chained_links(&error,
					 nop[0], nop[1], nop[2], nop[3]));
	g_assert(!error);
	g = pg_graph_new("graph1", nop[0], &error);
	g_assert(g && !error);
	g_assert(pg_graph_count(g) == 4);
	g_assert(!pg_graph_sanity(g, &error));
	g_assert(!error);

	/* graph is cut */
	g_assert(!pg_brick_unlink_edge(nop[1], nop[2], &error));
	g_assert(!error);
	g_assert(pg_graph_sanity(g, &error) == -1);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	g_assert(!pg_brick_link(nop[1], nop[2], &error));
	g_assert(!error);
	g_assert(!pg_graph_sanity(g, &error));

	/* graph is not fully explored */
	pg_graph_pop(g, "nop3");
	g_assert(pg_graph_sanity(g, &error) == -1);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	g_assert(!pg_graph_explore(g, NULL, &error));
	g_assert(!error);
	g_assert(!pg_graph_sanity(g, &error));
	g_assert(!error);

	/* graph is empty */
	pg_graph_destroy(g);
	g = pg_graph_new("empty", NULL, &error);
	g_assert(g && !error);
	g_assert(!pg_graph_sanity(g, &error));
	g_assert(!error);

	pg_graph_destroy(g);
}

static void test_graph_poll(void)
{
	/* let's have a graph with pollable queues
	 *
	 *                      __________
	 *                      |        |
	 * [queue0] ~ [queue1]--+        +--[queue4] ~ [queue5]
	 *                      |   hub  |
	 * [queue2] ~ [queue3]--+        |
	 *                      |________|
	 */
#	define NB_PKTS 64
	struct pg_graph *g, *trash;
	struct pg_error *error = NULL;
	struct pg_brick *queue[6], *hub;
	gchar *tmp;
	struct rte_mbuf *pkts[NB_PKTS];
	uint64_t mask = pg_mask_firsts(NB_PKTS);
	struct rte_mempool *mbuf_pool = pg_get_mempool();

	for (int i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 "F0:F1:F2:F3:F4:F5",
				 "E0:E1:E2:E3:E4:E5");
	}

	for (int i = 0; i < 6; i++) {
		tmp = g_strdup_printf("queue%i", i);
		queue[i] = pg_queue_new(tmp, 10, &error);
		g_assert(queue[i]);
		g_assert(!error);
		g_free(tmp);
	}
	hub = pg_hub_new("hub", 2, 2, &error);
	g_assert(hub);
	g_assert(!error);
	g_assert(!pg_queue_friend(queue[0], queue[1], &error));
	g_assert(!error);
	g_assert(!pg_queue_friend(queue[2], queue[3], &error));
	g_assert(!error);
	g_assert(!pg_queue_friend(queue[4], queue[5], &error));
	g_assert(!error);

	g_assert(!pg_brick_chained_links(&error, queue[1], hub, queue[4]));
	g_assert(!error);
	g_assert(!pg_brick_chained_links(&error, queue[3], hub));
	g_assert(!error);
	g = pg_graph_new("graph", hub, &error);
	g_assert(g && !error);
	g_assert(pg_graph_count(g) == 4);
	trash = pg_graph_new("trash", NULL, &error);
	g_assert(trash && !error);
	g_assert(!pg_graph_push(trash, queue[0], &error));
	g_assert(!error);
	g_assert(!pg_graph_push(trash, queue[2], &error));
	g_assert(!error);
	g_assert(!pg_graph_push(trash, queue[5], &error));
	g_assert(!error);

	for (int i = 0; i < 100; i++) {
		/* test with one burst */
		pg_brick_burst(queue[0], PG_EAST_SIDE, 0, pkts, mask, &error);
		g_assert(!pg_graph_poll(g, &error));
		g_assert(!error);
		g_assert(pg_queue_pressure(queue[0]) == 0);
		g_assert(pg_queue_pressure(queue[1]) == 0);
		g_assert(pg_queue_pressure(queue[2]) == 0);
		g_assert(pg_queue_pressure(queue[3]) == 25);
		g_assert(pg_queue_pressure(queue[4]) == 25);
		g_assert(pg_queue_pressure(queue[5]) == 0);

		/* unpressure queues */
		g_assert(!pg_graph_poll(trash, &error));
		g_assert(!error);
		g_assert(pg_queue_pressure(queue[0]) == 0);
		g_assert(pg_queue_pressure(queue[1]) == 0);
		g_assert(pg_queue_pressure(queue[2]) == 0);
		g_assert(pg_queue_pressure(queue[3]) == 0);
		g_assert(pg_queue_pressure(queue[4]) == 0);
		g_assert(pg_queue_pressure(queue[5]) == 0);

		/* everyone send a burst */
		pg_brick_burst(queue[0], PG_EAST_SIDE, 0, pkts, mask, &error);
		pg_brick_burst(queue[2], PG_EAST_SIDE, 0, pkts, mask, &error);
		pg_brick_burst(queue[5], PG_WEST_SIDE, 0, pkts, mask, &error);
		g_assert(!pg_graph_poll(g, &error));
		g_assert(!error);
		g_assert(pg_queue_pressure(queue[0]) == 0);
		g_assert(pg_queue_pressure(queue[1]) == 51);
		g_assert(pg_queue_pressure(queue[2]) == 0);
		g_assert(pg_queue_pressure(queue[3]) == 51);
		g_assert(pg_queue_pressure(queue[4]) == 51);
		g_assert(pg_queue_pressure(queue[5]) == 0);

		/* unpressure queues */
		g_assert(!pg_graph_poll(trash, &error));
		g_assert(!error);
		g_assert(pg_queue_pressure(queue[0]) == 0);
		g_assert(pg_queue_pressure(queue[1]) == 25);
		g_assert(pg_queue_pressure(queue[2]) == 0);
		g_assert(pg_queue_pressure(queue[3]) == 25);
		g_assert(pg_queue_pressure(queue[4]) == 25);
		g_assert(pg_queue_pressure(queue[5]) == 0);
		g_assert(!pg_graph_poll(trash, &error));
		g_assert(!error);
		g_assert(pg_queue_pressure(queue[0]) == 0);
		g_assert(pg_queue_pressure(queue[1]) == 0);
		g_assert(pg_queue_pressure(queue[2]) == 0);
		g_assert(pg_queue_pressure(queue[3]) == 0);
		g_assert(pg_queue_pressure(queue[4]) == 0);
		g_assert(pg_queue_pressure(queue[5]) == 0);
	}
	pg_graph_destroy(g);
	pg_graph_destroy(trash);
	pg_packets_free(pkts, mask);
#	undef NB_PKTS
}

static void test_graph_split_merge(void)
{
	/* Simple graph to split between nop1 and nop2
	 *
	 * [nop0]--[nop1]--[nop2]--[nop3]
	 *
	 * After split:
	 *
	 * [nop0]--[nop1]--[queue-west] ~ [queue-east]--[nop2]--[nop3]
	 *
	 */
	struct pg_graph *g, *east;
	struct pg_error *error = NULL;
	struct pg_brick *nop[4];
	gchar *tmp;

	for (int i = 0; i < 4; i++) {
		tmp = g_strdup_printf("nop%i", i);
		nop[i] = pg_nop_new(tmp, &error);
		g_free(tmp);
	}

	g_assert(!pg_brick_chained_links(&error,
					 nop[0], nop[1], nop[2], nop[3]));
	g_assert(!error);
	g = pg_graph_new("graph", nop[0], &error);
	g_assert(g && !error);
	g_assert(pg_graph_count(g) == 4);
	g_assert(!pg_graph_sanity(g, &error));
	g_assert(!error);

	/* split graph */
	east = pg_graph_split(g,
			      "east-graph",
			      "nop1",
			      "nop2",
			      "queue-west",
			      "queue-east",
			      &error);
	g_assert(east);
	g_assert(!error);
	g_assert(pg_graph_count(g) == 3);
	g_assert(pg_graph_count(east) == 3);
	g_assert(pg_graph_get(g, "nop0") == nop[0]);
	g_assert(pg_graph_get(g, "nop1") == nop[1]);
	g_assert(pg_graph_get(g, "queue-west"));
	g_assert(pg_graph_get(east, "nop2") == nop[2]);
	g_assert(pg_graph_get(east, "nop3") == nop[3]);
	g_assert(pg_graph_get(east, "queue-east"));
	g_assert(pg_queue_are_friend(pg_graph_get(g, "queue-west"),
				     pg_graph_get(east, "queue-east")));
	g_assert(!pg_graph_sanity(g, &error));
	g_assert(!error);
	g_assert(!pg_graph_sanity(east, &error));
	g_assert(!error);

	/* let's remerge it now ! */
	g_assert(!pg_graph_merge(g, east, &error));
	g_assert(!error);
	g_assert(pg_graph_count(g) == 4);
	g_assert(!pg_graph_sanity(g, &error));
	g_assert(!error);
	g_assert(pg_graph_get(g, "nop0") == nop[0]);
	g_assert(pg_graph_get(g, "nop1") == nop[1]);
	g_assert(pg_graph_get(g, "nop2") == nop[2]);
	g_assert(pg_graph_get(g, "nop3") == nop[3]);

	pg_graph_destroy(g);
}

static void test_graph_complex_merge(void)
{
	/* Let's merge two graph with multiple queues.
	 * This graph is absurd but that's just a test.
	 *         ___________                       ___________
	 *         |         |                       |         |
	 * [nop0]--+         +--[queue0] ~ [queue3]--+         +--[nop3]
	 *         |         |                       |         |
	 * [nop1]--+ switch0 +--[queue1] ~ [queue4]--+ switch1 +--[nop4]
	 *         |         |                       |         |
	 * [nop2]--+         +--[queue2] ~ [queue5]--+         +--[nop5]
	 *         |_________|                       |_________|
	 *
	 * After merge, switch0 and switch 1 must be collected on 3 ports
	 *         ___________    ___________
	 *         |         |    |         |
	 * [nop0]--+         +----+         +--[nop3]
	 *         |         |    |         |
	 * [nop1]--+ switch0 +----+ switch1 +--[nop4]
	 *         |         |    |         |
	 * [nop2]--+         +----+         +--[nop5]
	 *         |_________|    |_________|
	 *
	 */
	struct pg_graph *g_west, *g_east;
	struct pg_error *error = NULL;
	struct pg_brick *queue[6], *sw[2], *nop[6];
	char *tmp;

	for (int i = 0; i < 6; i++) {
		tmp = g_strdup_printf("queue%i", i);
		queue[i] = pg_queue_new(tmp, 0, &error);
		g_assert(queue[i]);
		g_assert(!error);
		g_free(tmp);
		tmp = g_strdup_printf("nop%i", i);
		nop[i] = pg_nop_new(tmp, &error);
		g_assert(nop[i]);
		g_assert(!error);
		g_free(tmp);
	}
	sw[0] = pg_switch_new("switch0", 3, 3, PG_WEST_SIDE, &error);
	g_assert(sw[0]);
	g_assert(!error);
	sw[1] = pg_switch_new("switch1", 3, 3, PG_WEST_SIDE, &error);
	g_assert(sw[1]);
	g_assert(!error);

	g_assert(!pg_brick_chained_links(&error, nop[0], sw[0], queue[0]));
	g_assert(!error);
	g_assert(!pg_brick_chained_links(&error, nop[1], sw[0], queue[1]));
	g_assert(!error);
	g_assert(!pg_brick_chained_links(&error, nop[2], sw[0], queue[2]));
	g_assert(!error);
	g_assert(!pg_brick_chained_links(&error, queue[3], sw[1], nop[3]));
	g_assert(!error);
	g_assert(!pg_brick_chained_links(&error, queue[4], sw[1], nop[4]));
	g_assert(!error);
	g_assert(!pg_brick_chained_links(&error, queue[5], sw[1], nop[5]));
	g_assert(!error);

	g_assert(!pg_queue_friend(queue[0], queue[3], &error));
	g_assert(!error);
	g_assert(!pg_queue_friend(queue[1], queue[4], &error));
	g_assert(!error);
	g_assert(!pg_queue_friend(queue[2], queue[5], &error));
	g_assert(!error);

	g_west = pg_graph_new("graph-west", sw[0], &error);
	g_assert(g_west && !error);
	g_assert(pg_graph_count(g_west) == 7);
	g_east = pg_graph_new("graph-east", sw[1], &error);
	g_assert(g_west && !error);
	g_assert(pg_graph_count(g_west) == 7);
	g_assert(!pg_graph_sanity(g_west, &error));
	g_assert(!error);
	g_assert(!pg_graph_sanity(g_east, &error));
	g_assert(!error);

	g_assert(!pg_graph_merge(g_west, g_east, &error));
	g_assert(!error);
	g_assert(pg_graph_count(g_west) == 8);
	g_assert(!pg_graph_sanity(g_west, &error));
	g_assert(!error);
	for (int i = 0; i < 6; i++) {
		tmp = g_strdup_printf("nop%i", i);
		g_assert(pg_graph_get(g_west, tmp) == nop[i]);
		g_free(tmp);
	}
	g_assert(pg_graph_get(g_west, "switch0") == sw[0]);
	g_assert(pg_graph_get(g_west, "switch1") == sw[1]);
	g_assert(pg_brick_refcount(sw[0]) == 7);
	g_assert(pg_brick_refcount(sw[1]) == 7);

	pg_graph_destroy(g_west);
}

void test_graph(void)
{
	g_test_add_func("/core/graph/lifecycle", test_graph_lifecycle);
	g_test_add_func("/core/graph/explore", test_graph_explore);
	g_test_add_func("/core/graph/sanity", test_graph_sanity);
	g_test_add_func("/core/graph/poll", test_graph_poll);
	g_test_add_func("/core/graph/complex-merge", test_graph_complex_merge);
	g_test_add_func("/core/graph/split-merge", test_graph_split_merge);
}
