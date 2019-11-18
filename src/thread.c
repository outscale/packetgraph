/* Copyright 2017 Outscale SAS
 *
 * This file is part of Packetgraph.
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

#include <rte_config.h>
#include <rte_branch_prediction.h>
#include <rte_atomic.h>
#include <rte_lcore.h>

#include <glib.h>
#include <unistd.h>
#include <packetgraph/thread.h>

#include "utils/stack.h"

const char *pg_thread_str_states[PG_THREAD_RUNNING + 1] = {
	"PG_THREAD_STOPPED",
	"PG_THREAD_BROKEN",
	"PG_THREAD_RUNNING"
};

#if RTE_MAX_LCORE > PG_STACK_BLOCK_SIZE
#define PG_THREAD_MAX PG_STACK_BLOCK_SIZE
#else
#define PG_THREAD_MAX RTE_MAX_LCORE
#endif

static STACK_CREATE(free_thread_ids, int16);

enum pg_thread_op {
	PG_THREAD_NONE,
	PG_THREAD_ANSWER,
	PG_THREAD_STATE,
	PG_THREAD_ADD_GRAPH,
	PG_THREAD_RM_GRAPH,
	PG_THREAD_RUN,
	PG_THREAD_POP_ERROR,
	PG_THREAD_FORCE_START,
	PG_THREAD_STOP,
	PG_THREAD_QUIT,
};

#define PG_THREAD_MAX_OP 128

union pg_thread_arg {
	void *ptr;
	uint64_t u64;
	int16_t i16;
	int32_t i32;
	struct {
		int32_t i32_1;
		int32_t i32_2;
	};
};

struct pg_thread_client {
	int id;
	rte_atomic16_t is_queue_locked;
	rte_atomic16_t is_queue_empty;
	int queue_size;
	struct thread_op {
		enum pg_thread_op op;
		union pg_thread_arg arg;
		union pg_thread_arg arg2;
	} queue[PG_THREAD_MAX_OP];
};

struct pg_thread_client *thread_ids[PG_THREAD_MAX];

static bool is_init;
static int16_t threads_max;
static int16_t last_thread_id;
static int16_t nb_thread_id_used;
static _Thread_local int queue_pos;

static inline int try_lock(struct pg_thread_client *client)
{
	return rte_atomic16_test_and_set(&client->is_queue_locked);
}

static inline struct thread_op *get_thread_op(struct pg_thread_client *client)
{
	return &client->queue[queue_pos];
}

static inline enum pg_thread_op
dequeue_op_nolock(struct pg_thread_client *client)
{
	return client->queue[queue_pos].op;
}

static inline void relock(struct pg_thread_client *client)
{
	rte_atomic16_clear(&client->is_queue_locked);
}

static int pg_thread_main(void *a)
{
	int id = rte_lcore_id();
	struct pg_thread_client *client = thread_ids[id];
	enum pg_thread_state state = PG_THREAD_STOPPED;
	struct thread_op *cur;
	struct pg_graph *graphs[PG_STACK_BLOCK_SIZE];
	uint8_t started[PG_STACK_BLOCK_SIZE];
	int last_graph = 0;
	struct pg_error *error;
	int ret;

	STACK_CREATE(free_graph, int8);
	STACK_CREATE(errors, ptr);
	STACK_CREATE(errors_gid, int16);

	stack_set_limit(errors, 128);
	stack_set_limit(errors_gid, 128);
	while (1) {
		if (try_lock(client)) {
			queue_pos = client->queue_size - 1;
			if (queue_pos < 0) {
				relock(client);
				continue;
			}
again:
			switch (dequeue_op_nolock(client)) {
			case PG_THREAD_QUIT:
				stack_destroy(free_graph);
				stack_destroy(errors);
				stack_destroy(errors_gid);
				relock(client);
				return 0;
			case PG_THREAD_POP_ERROR:
				cur = get_thread_op(client);
				cur->arg.ptr = stack_pop(errors, NULL);
				cur->arg2.i32_1 = stack_pop(errors_gid, -1);
				cur->arg2.i32_2 = stack_len(errors_gid);
				cur->op = PG_THREAD_ANSWER;
				break;
			case PG_THREAD_RM_GRAPH:
				cur = get_thread_op(client);
				ret = cur->arg.i16;
				stack_push(free_graph, (int8_t)ret);
				cur->arg.ptr = graphs[ret];
				started[ret] = PG_THREAD_STOPPED;
				cur->op = PG_THREAD_ANSWER;
				break;
			case PG_THREAD_ADD_GRAPH:
				cur = get_thread_op(client);
				ret = stack_pop(free_graph, -1);
				if (ret == -1) {
					ret = last_graph;
					if (ret == PG_STACK_BLOCK_SIZE) {
						cur->op = PG_THREAD_ANSWER;
						cur->arg.i32 = -1;
						continue;
					}
					last_graph += 1;
				}
				graphs[ret] = cur->arg.ptr;
				started[ret] = PG_THREAD_RUNNING;
				cur->arg.i32 = ret;
				cur->op = PG_THREAD_ANSWER;
				break;
			case PG_THREAD_FORCE_START:
				cur = get_thread_op(client);
				started[cur->arg.i32] = PG_THREAD_RUNNING;
				ret = 0;

				for (int i = 0; i < last_graph; ++i) {
					if (started[i] == PG_THREAD_BROKEN)
						++ret;
				}
				if (!ret && !stack_len(errors))
					state = PG_THREAD_RUNNING;
				cur->op = PG_THREAD_NONE;
				break;
			case PG_THREAD_RUN:
				if (!last_graph)
					break;
				cur = get_thread_op(client);
				for (int i = 0; i < last_graph; ++i) {
					if (started[i] == PG_THREAD_BROKEN)
						started[i] = PG_THREAD_RUNNING;
				}
				state = PG_THREAD_RUNNING;
				cur->op = PG_THREAD_NONE;
				break;
			case PG_THREAD_STOP:
				cur = get_thread_op(client);
				state = PG_THREAD_STOPPED;
				cur->op = PG_THREAD_ANSWER;
				break;
			case PG_THREAD_STATE:
				cur = get_thread_op(client);
				cur->arg.u64 = state;
				cur->op = PG_THREAD_ANSWER;
				break;
			case PG_THREAD_NONE:
			case PG_THREAD_ANSWER:
				break;
			}

			queue_pos -= 1;
			if (queue_pos > 0)
				goto again;

			relock(client);
		}
		if (unlikely(state == PG_THREAD_STOPPED)) {
			sched_yield();
			continue;
		}
		for (int j = 0; j < 32; ++j) {
			for (int16_t i = 0; i < last_graph; ++i) {
				if (started[i] != PG_THREAD_RUNNING)
					continue;
				if (unlikely(pg_graph_poll(graphs[i],
							   &error) < 0)) {
					state = PG_THREAD_BROKEN;
					started[i] = PG_THREAD_BROKEN;
					stack_push(errors, (void *)error);
					stack_push(errors_gid, i);
				}
			}
		}
	}
	return 0;
}

int16_t pg_thread_max(void)
{
	if (!is_init) {
		/*
		 * As rte_lcore_count equal numbers of slave + master
		 * we need to substract 1, because we are on master
		 */
		threads_max = rte_lcore_count() - 1;
		if (threads_max > PG_THREAD_MAX)
			threads_max = PG_THREAD_MAX;
	}

	return threads_max;
}

int16_t pg_thread_init(struct pg_error **errp)
{
	int ret = stack_pop(free_thread_ids, -1);

	/* call pg_thread_max to init threads_max */
	pg_thread_max();
	if (ret == -1) {
		ret = rte_get_next_lcore(last_thread_id, 1, 0);
		if (unlikely(nb_thread_id_used + 1 > threads_max)) {
			*errp = pg_error_new("pg_thread limit reach(%d), %s",
					     threads_max,
					     "see dpdk lcore config");
			return -1;
		}
		last_thread_id = ret;
	}
	nb_thread_id_used += 1;
	thread_ids[ret] = g_new0(struct pg_thread_client, 1);
	rte_atomic16_init(&thread_ids[ret]->is_queue_locked);
	rte_atomic16_init(&thread_ids[ret]->is_queue_empty);
	rte_atomic16_set(&thread_ids[ret]->is_queue_empty, 1);
	rte_atomic16_clear(&thread_ids[ret]->is_queue_locked);
	thread_ids[ret]->id = ret;
	rte_eal_remote_launch(pg_thread_main, NULL, ret);
	return ret;
}

#define TYPE_NAME ptr
#include "thread-impl.h"
#undef TYPE_NAME
#define TYPE_NAME i16
#include "thread-impl.h"
#undef TYPE_NAME
#define TYPE_NAME i32
#include "thread-impl.h"
#undef TYPE_NAME

#define enqueue(thread_id, op, arg)		\
	(_Generic((arg), void * : enqueue_ptr,	\
		  int16_t : enqueue_i16,	\
		  int32_t : enqueue_i32		\
		)(thread_id, op, arg))


#define wait_enqueue(thread_id, op, arg)		\
	(_Generic((arg), void * : wait_enqueue_ptr,	\
		  struct pg_graph * : wait_enqueue_ptr,	\
		  int16_t : wait_enqueue_i16,		\
		  int32_t : wait_enqueue_i32		\
		)(thread_id, op, arg))


static void shrink_queue(struct pg_thread_client *client, int should_lock)
{
	int i;

	if (should_lock)
		while (!rte_atomic16_test_and_set(&client->is_queue_locked))
			;

	for (i = client->queue_size; i > 0 &&
		     client->queue[i - 1].op == PG_THREAD_NONE; --i)
		;
	client->queue_size = i;
	if (!client->queue_size)
		rte_atomic16_set(&client->is_queue_empty, 1);
	if (should_lock)
		rte_atomic16_clear(&client->is_queue_locked);
}

static struct thread_op *try_get_answer(struct pg_thread_client *client,
					int queue_idx)
{
	if (!rte_atomic16_test_and_set(&client->is_queue_locked))
		return NULL;
	if (client->queue[queue_idx].op == PG_THREAD_ANSWER)
		return &client->queue[queue_idx];
	g_assert(client->queue[queue_idx].op != PG_THREAD_NONE);
	rte_atomic16_clear(&client->is_queue_locked);
	return NULL;
}

static struct thread_op *wait_answer(int16_t thread_id, int queue_id)
{
	struct thread_op *cur_op;

	while (!(cur_op = try_get_answer(thread_ids[thread_id], queue_id)))
		sched_yield();
	return cur_op;
}

enum pg_thread_state pg_thread_state(int16_t thread_id)
{
	struct pg_thread_client *client = thread_ids[thread_id];
	int queue_id;
	enum pg_thread_state ret;
	struct thread_op *cur_op;

	queue_id = wait_enqueue(thread_id, PG_THREAD_STATE, NULL);
	sched_yield();
	cur_op = wait_answer(thread_id, queue_id);
	ret = cur_op->arg.u64;
	cur_op->op = PG_THREAD_NONE;
	shrink_queue(client, 0);
	rte_atomic16_clear(&client->is_queue_locked);
	return ret;
}

int pg_thread_run(int16_t thread_id)
{
	wait_enqueue(thread_id, PG_THREAD_RUN, NULL);
	return 0;
}

int pg_thread_stop(int16_t thread_id)
{
	struct thread_op *cur_op;
	int queue_id = wait_enqueue(thread_id, PG_THREAD_STOP, NULL);

	cur_op = wait_answer(thread_id, queue_id);
	shrink_queue(thread_ids[thread_id], 0);
	cur_op->op = PG_THREAD_NONE;
	rte_atomic16_clear(&thread_ids[thread_id]->is_queue_locked);
	return 0;
}

int pg_thread_add_graph(int16_t thread_id, struct pg_graph *graph)
{
	struct pg_thread_client *client = thread_ids[thread_id];
	int queue_id;
	enum pg_thread_state ret;
	struct thread_op *cur_op;

	if (unlikely(!graph))
		return -1;
	queue_id = wait_enqueue(thread_id, PG_THREAD_ADD_GRAPH, graph);
	sched_yield();
	cur_op = wait_answer(thread_id, queue_id);
	ret = cur_op->arg.i32;
	cur_op->op = PG_THREAD_NONE;
	shrink_queue(client, 0);
	rte_atomic16_clear(&client->is_queue_locked);
	return ret;

}

int pg_thread_pop_error(int16_t thread_id, int *graph_id,
			struct pg_error **errp)
{
	struct pg_thread_client *client = thread_ids[thread_id];
	int queue_id;
	int ret;
	struct thread_op *cur_op;

	queue_id = wait_enqueue(thread_id, PG_THREAD_POP_ERROR, NULL);
	sched_yield();
	cur_op = wait_answer(thread_id, queue_id);
	*errp = cur_op->arg.ptr;
	*graph_id = cur_op->arg2.i32_1;
	ret = *graph_id < 0 ? -1 : cur_op->arg2.i32_2;
	cur_op->op = PG_THREAD_NONE;
	shrink_queue(client, 0);
	rte_atomic16_clear(&client->is_queue_locked);
	return ret;
}

int pg_thread_force_start_graph(int16_t thread_id, int32_t graph_id)
{
	wait_enqueue(thread_id, PG_THREAD_FORCE_START, graph_id);
	sched_yield();
	return 0;
}

struct pg_graph *pg_thread_pop_graph(int16_t thread_id, int32_t graph_id)
{
	struct pg_thread_client *client = thread_ids[thread_id];
	int queue_id;
	struct pg_graph *ret;
	struct thread_op *cur_op;

	queue_id = wait_enqueue(thread_id, PG_THREAD_RM_GRAPH, graph_id);
	sched_yield();
	cur_op = wait_answer(thread_id, queue_id);
	ret = cur_op->arg.ptr;
	cur_op->op = PG_THREAD_NONE;
	shrink_queue(client, 0);
	rte_atomic16_clear(&client->is_queue_locked);
	return ret;
}

int pg_thread_destroy(int16_t thread_id)
{
	if (unlikely(thread_id > threads_max || !thread_ids[thread_id]))
		return -1;
	enqueue(thread_id, PG_THREAD_QUIT, NULL);
	rte_eal_wait_lcore(thread_id);
	nb_thread_id_used -= 1;
	g_free(thread_ids[thread_id]);
	thread_ids[thread_id] = NULL;
	if (!nb_thread_id_used) {
		stack_destroy(free_thread_ids);
		last_thread_id = 0;
	} else {
		stack_push(free_thread_ids, thread_id);
	}
	return 0;
}
