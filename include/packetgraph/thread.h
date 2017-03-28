/* Copyright 2017 Outscale SAS
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

/**
 * @file Packetgraph's thread API let users run one or more graph in thread
 */

#ifndef _PG_THREAD_H
#define _PG_THREAD_H

#include <packetgraph/errors.h>
#include <packetgraph/graph.h>

enum pg_thread_state {
	/* The graph has not started yet or has been stopped */
	PG_THREAD_STOPPED,
	/* This state occurs when one or more graph has stopped running due
	 * to an error.
	 * Non erroneous graph are still running.
	 * When this case occurs,
	 * you should check errors with pg_thread_pop_error */
	PG_THREAD_BROKEN,
	/* Everything is normal */
	PG_THREAD_RUNNING
};

#define pg_thread_states_to_str(state)		\
	(pg_thread_str_states[state + 1])
extern const char *pg_thread_str_states[PG_THREAD_RUNNING + 1];

/**
 * @return thread id (tid) on sucess, -1 on failure
 */
int16_t pg_thread_init(struct pg_error **errp);

/**
 * @return the maximum number of threads, which is the same as the last tid.
 */
int16_t pg_thread_max(void);

/**
 * start to pool packets on every graph added previously to
 * the thread with pg_thread_add_graph, regardless if a graph is in a broken
 * state.
 * @return 0 on sucess, -1 on failure
 */
int pg_thread_run(int16_t thread_id);

/**
 * stop pooling packets
 * @return 0 on sucess, -1 on failure
 */
int pg_thread_stop(int16_t thread_id);

/**
 * @return thread's state
 */
enum pg_thread_state pg_thread_state(int16_t thread_id);

/**
 * when an error occurs in the thread, the erroneous graph stops and an error
 * is pushed on a stack.
 * This function pop the last error on the stack.
 * @graph_id pointer where to write the id of the graph who produced
 * the last error
 * @errp pointer where to write the last error of the stack
 * @return the number of error left on the stack.
 * 0 for the last error. -1 if there were no error on the stack
 */
int pg_thread_pop_error(int16_t tid, int *graph_id,
			struct pg_error **errp);

/**
 * Restart a graph in a broken state
 * @return 0 on success, -1 on failure
 */
int pg_thread_force_start_graph(int16_t tid, int32_t graph_id);

/**
 * add @graph to the thread designate by @tid, the graph is stop by default
 * @return graph id on success, -1 on failure
 */
int pg_thread_add_graph(int16_t tid, struct pg_graph *graph);

struct pg_graph *pg_thread_pop_graph(int16_t tid, int32_t graph_id);

int pg_thread_destroy(int16_t tid);

#endif
