/* Copyright 2017 Outscale SAS
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

#include <glib.h>
#include <packetgraph/packetgraph.h>
#include <unistd.h>
#include "utils/tests.h"
#include "collect.h"
#include "fail.h"

static void test_threads_lifecycle(void)
{
	struct pg_error *error = NULL;
	int tid = pg_thread_init(&error);

	g_assert(pg_thread_max() > 0);
	g_assert(tid == 1);
	g_assert(!error);
	g_assert(pg_thread_state(tid) == PG_THREAD_STOPPED);
	pg_thread_destroy(tid);

	for (int i = 0; i < pg_thread_max(); ++i) {
		tid = pg_thread_init(&error);
		g_assert(tid == i + 1);
		g_assert(!error);
	}
	tid = pg_thread_init(&error);
	g_assert(tid < 0);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	if (pg_thread_max() > 1) {
		pg_thread_destroy(2);
		g_assert(pg_thread_init(&error) == 2);
		g_assert(!error);
	}
	pg_thread_destroy(1);
	g_assert(pg_thread_init(&error) == 1);
	g_assert(!error);

	for (int i = 0; i < pg_thread_max(); ++i)
		pg_thread_destroy(i + 1);

	for (int i = 0; i < 3000; ++i) {
		tid = pg_thread_init(&error);
		g_assert(tid == 1);
		g_assert(!error);
		g_assert(pg_thread_state(tid) == PG_THREAD_STOPPED);
		pg_thread_destroy(tid);
	}
}

static void tx_callback(struct pg_brick *brick, pg_packet_t **tx_burst,
			uint16_t *tx_burst_len, void *private_data)
{
	pg_packet_set_len(tx_burst[0], 100);
	*tx_burst_len = 1;
}

static void test_threads_run(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *col = pg_collect_new("col", &error);
	struct pg_brick *rxtx = pg_rxtx_new("rxtx", NULL, tx_callback, &error);
	struct pg_graph *graph;
	int tid = pg_thread_init(&error);

	g_assert(col && rxtx);
	pg_brick_link(rxtx, col, &error);
	graph = pg_graph_new("graph", rxtx, &error);
	g_assert(tid >= 1);
	g_assert(!error);
	g_assert(!pg_thread_add_graph(tid, graph));
	g_assert(pg_thread_state(tid) == PG_THREAD_STOPPED);
	pg_thread_run(tid);
	usleep(1);
	g_assert(pg_thread_state(tid) == PG_THREAD_RUNNING);
	pg_thread_stop(tid);
	g_assert(pg_thread_state(tid) == PG_THREAD_STOPPED);
	g_assert(pg_thread_pop_graph(tid, 0));
	g_assert(!pg_thread_add_graph(tid, graph));

	g_assert(pg_brick_tx_bytes(rxtx) > 100);
	pg_thread_destroy(tid);
	pg_graph_destroy(graph);
}

static void test_threads_errors(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *col = pg_fail_new("col", &error);
	struct pg_brick *rxtx = pg_rxtx_new("rxtx", NULL, tx_callback, &error);
	struct pg_graph *graph;
	int tid = pg_thread_init(&error);
	int gid;

	g_assert(col && rxtx);
	pg_brick_link(rxtx, col, &error);
	graph = pg_graph_new("graph", rxtx, &error);
	g_assert(tid >= 1);
	g_assert(!error);
	gid = pg_thread_add_graph(tid, graph);
	g_assert(gid == 0);
	g_assert(pg_thread_state(tid) == PG_THREAD_STOPPED);

	g_assert(!pg_thread_run(tid));
	usleep(1);
	g_assert(pg_thread_state(tid) == PG_THREAD_BROKEN);
	g_assert(pg_brick_tx_bytes(rxtx) == 100);
	pg_thread_force_start_graph(tid, gid);
	usleep(1);
	g_assert(pg_thread_state(tid) == PG_THREAD_BROKEN);
	g_assert(pg_thread_pop_error(tid, &gid, &error) == 1);
	g_assert(!gid);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	g_assert(pg_thread_pop_error(tid, &gid, &error) == 0);
	g_assert(!gid);
	g_assert(error);
	pg_error_free(error);
	error = NULL;
	g_assert(pg_thread_pop_error(tid, &gid, &error) == -1);
	g_assert(gid == -1);
	g_assert(pg_brick_tx_bytes(rxtx) == 200);
	g_assert(!error);
	pg_graph_brick_destroy(graph, "col", &error);
	g_assert(!error);
	pg_thread_force_start_graph(tid, 0);
	usleep(1);
	g_assert(pg_thread_state(tid) == PG_THREAD_RUNNING);
	pg_thread_stop(tid);
	g_assert(pg_thread_state(tid) == PG_THREAD_STOPPED);
	g_assert(pg_brick_tx_bytes(rxtx) > 300);
	pg_thread_destroy(tid);
}

int main(int argc, char **argv)
{
	int ret;
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	ret = pg_start(argc, argv);
	g_assert(ret >= 0);
	g_assert(!pg_init_seccomp());

	pg_test_add_func("/threads/lifecycle", test_threads_lifecycle);
	pg_test_add_func("/threads/run", test_threads_run);
	pg_test_add_func("/threads/error", test_threads_errors);

	return g_test_run();
}
