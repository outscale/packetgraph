/* Copyright 2016 Outscale SAS
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
#include "brick-int.h"
#include "graph-int.h"

static void brick_destroy_cb(gpointer data)
{
	pg_brick_destroy(data);
}

static inline void empty_graph(struct pg_graph *graph)
{
	g_hash_table_steal_all(graph->all);
	g_slist_free(graph->pollable);
	graph->pollable = NULL;
}

struct pg_graph *pg_graph_new(const char *name, struct pg_brick *explore,
			      struct pg_error **error)
{
	struct pg_graph *ret = g_new(struct pg_graph, 1);

	ret->name = g_strdup(name);
	ret->all = g_hash_table_new_full(g_str_hash, g_str_equal,
					 NULL, &brick_destroy_cb);
	ret->pollable = NULL;
	if (explore && pg_graph_explore_ptr(ret, explore, error) < 0) {
		empty_graph(ret);
		pg_graph_destroy(ret);
		return NULL;
	}

	return ret;
}

const char *pg_graph_name(struct pg_graph *graph)
{
	return graph->name;
}

void pg_graph_destroy(struct pg_graph *graph)
{
	g_free(graph->name);
	g_hash_table_destroy(graph->all);
	g_slist_free(graph->pollable);
	g_free(graph);
}

int pg_graph_explore(struct pg_graph *graph,
		     const char *brick_name,
		     struct pg_error **error)
{
	struct pg_brick *b = NULL;

	if (brick_name) {
		b = pg_graph_get(graph, brick_name);
		if (!b) {
			*error = pg_error_new("brick %s not found", brick_name);
			return -1;
		}
	}
	return pg_graph_explore_ptr(graph, b, error);
}

static inline GList *explore_add_neighbours(GList *todo,
					    GList *done,
					    struct pg_brick *b)
{
	PG_BRICK_FOREACH_EDGES(b, it) {
		struct pg_brick *n = pg_brick_edge_iterator_get(&it)->link;

		if (n && !g_list_find(todo, n) && !g_list_find(done, n))
			todo = g_list_prepend(todo, n);
	}
	return todo;
}

static inline struct pg_brick *get_any_brick(struct pg_graph *graph)
{
	struct pg_brick *brick = NULL;

	if (graph->pollable) {
		brick = graph->pollable->data;
	} else if (g_hash_table_size(graph->all)) {
		GHashTableIter iter;
		gpointer key;

		g_hash_table_iter_init(&iter, graph->all);
		g_hash_table_iter_next(&iter, &key, (void **)&brick);
	}
	return brick;
}

int pg_graph_explore_ptr(struct pg_graph *graph,
			 struct pg_brick *brick,
			 struct pg_error **error)
{
	if (!brick) {
		brick = get_any_brick(graph);
		if (!brick)
			return 0;
	}
	/* first, clear all graph */
	empty_graph(graph);

	/* explore new graph from brick */
	GList *todo = NULL;
	GList *done = NULL;

	todo = explore_add_neighbours(todo, done, brick);
	if (!todo) {
		done = g_list_prepend(done, brick);
	} else {
		while (todo) {
			struct pg_brick *b = todo->data;

			todo = g_list_remove(todo, b);
			done = g_list_prepend(done, b);
			todo = explore_add_neighbours(todo, done, b);
		}
	}

	/* add all bricks to graph */
	int ret = 0;
	GList *h = done;

	while (h) {
		if (pg_graph_push(graph, h->data, error) < 0) {
			ret = -1;
			break;
		}
		h = g_list_next(h);
	}
	g_list_free(done);
	return ret;
}

int pg_graph_poll(struct pg_graph *graph, struct pg_error **error)
{
	GSList *n = graph->pollable;
	uint16_t count;

	while (n) {
		if (pg_brick_poll(n->data, &count, error) < 0) {
			if (!*error)
				*error = pg_error_new("Cannot poll %s",
					((struct pg_brick *) n->data)->name);
			return -1;
		}
		n = g_slist_next(n);
	}
	return 0;
}

int pg_graph_sanity(struct pg_graph *graph, struct pg_error **error)
{
	struct pg_brick *brick = get_any_brick(graph);

	if (!brick)
		return 0;
	struct pg_graph *g = pg_graph_new("-", brick, error);
	int ret = -1;

	if (!g)
		return -1;
	if (pg_graph_count(g) > pg_graph_count(graph))
		*error = pg_error_new("Graph not fully explored");
	else if (pg_graph_count(g) < pg_graph_count(graph))
		*error = pg_error_new("Graph looks splitted");
	else
		ret = 0;
	empty_graph(g);
	pg_graph_destroy(g);
	return ret;
}

struct pg_brick *pg_graph_get(struct pg_graph *graph, const char *brick_name)
{
	struct pg_brick *b;

	if (g_hash_table_lookup_extended(graph->all, brick_name,
					 NULL, (void **) &b))
		return b;
	return NULL;
}

static inline struct pg_brick *graph_pop(struct pg_graph *graph,
					 struct pg_brick *b)
{
	if (!b)
		return b;
	g_hash_table_steal(graph->all, b->name);
	if (b->poll)
		graph->pollable = g_slist_remove(graph->pollable, b);
	return b;
}

struct pg_brick *pg_graph_unsafe_pop(struct pg_graph *graph,
				     const char *brick_name)
{
	return graph_pop(graph, pg_graph_get(graph, brick_name));
}

int pg_graph_push(struct pg_graph *graph,
		  struct pg_brick *brick,
		  struct pg_error **error)
{
	if (g_hash_table_lookup_extended(graph->all, brick->name, NULL, NULL)) {
		*error = pg_error_new("A brick named %s already in graph %s",
				      brick->name, graph->name);
		return -1;
	}
	g_hash_table_insert(graph->all, brick->name, brick);
	if (brick->poll)
		graph->pollable = g_slist_prepend(graph->pollable, brick);
	return 0;
}

int pg_graph_count(struct pg_graph *graph)
{
	return g_hash_table_size(graph->all);
}

int pg_graph_brick_destroy(struct pg_graph *graph,
			   const char *brick_name,
			   struct pg_error **error)
{
	struct pg_brick *b = pg_graph_unsafe_pop(graph, brick_name);

	if (!b) {
		*error = pg_error_new("Brick %s not found", brick_name);
		return -1;
	}
	pg_brick_destroy(b);
	return 0;
}

struct pg_graph *pg_graph_split(struct pg_graph *graph,
				const char *east_graph_name,
				const char *west_brick_name,
				const char *east_brick_name,
				const char *west_queue_name,
				const char *east_queue_name,
				struct pg_error **error)
{
	struct pg_brick *w = pg_graph_get(graph, west_brick_name);
	struct pg_brick *e = pg_graph_get(graph, east_brick_name);
	struct pg_brick *qe, *qw;

	if (pg_brick_unlink_edge(w, e, error) < 0)
		return NULL;
	qw = pg_queue_new(west_queue_name, 0, error);
	if (!qw)
		return NULL;
	qe = pg_queue_new(east_queue_name, 0, error);
	if (!qe)
		return NULL;
	if (pg_brick_link(w, qw, error) < 0)
		return NULL;
	if (pg_brick_link(qe, e, error) < 0)
		return NULL;
	if (pg_queue_friend(qw, qe, error) < 0)
		return NULL;
	if (pg_graph_explore_ptr(graph, qw, error) < 0)
		return NULL;
	return pg_graph_new(east_graph_name, qe, error);
}

static inline GList *get_all_queues(struct pg_graph *g)
{
	GList *ret = NULL;
	GSList *h = g->pollable;

	while (h) {
		if (g_str_equal(pg_brick_type(h->data), "queue"))
			ret = g_list_prepend(ret, h->data);
		h = g_slist_next(h);
	}
	return ret;
}

static inline int merge_queues(struct pg_brick *q1,
			       struct pg_brick *q2,
			       struct pg_error **error)
{
	struct pg_brick *q1_pair = q1->side.edge.link;
	struct pg_brick *q2_pair = q2->side.edge.link;

	pg_queue_unfriend(q1);
	if (q1_pair) {
		pg_brick_unlink(q1, error);
		if (pg_error_is_set(error))
			return -1;
	}
	if (q2_pair) {
		pg_brick_unlink(q2, error);
		if (pg_error_is_set(error))
			return -1;
	}
	if (q1_pair && q2_pair && pg_brick_link(q1_pair, q2_pair, error) < 0)
		return -1;
	return 0;
}

int pg_graph_merge(struct pg_graph *graph_west,
		   struct pg_graph *graph_east,
		   struct pg_error **error)
{
	GList *q_west, *q_east, *hw, *he, *destroy_w = NULL, *destroy_e = NULL;

	/* populate all queue bricks of graph_west */
	q_west = get_all_queues(graph_west);
	q_east = get_all_queues(graph_east);

	/* merge matching queues */
	hw = q_west;
	while (hw) {
		he = q_east;
		while (he) {
			if (pg_queue_are_friend(hw->data, he->data)) {
				if (merge_queues(hw->data, he->data,
						 error) < 0)
					return -1;
				destroy_w = g_list_prepend(destroy_w, hw->data);
				destroy_e = g_list_prepend(destroy_e, he->data);
				break;
			}
			he = g_list_next(he);
		}
		hw = g_list_next(hw);
	}

	/* pop and destroy old queue bricks */
	hw = destroy_w;
	while (hw) {
		graph_pop(graph_west, hw->data);
		pg_brick_destroy(hw->data);
		hw = g_list_next(hw);
	}
	he = destroy_e;
	while (he) {
		graph_pop(graph_east, he->data);
		pg_brick_destroy(he->data);
		he = g_list_next(he);
	}
	g_list_free(q_west);
	g_list_free(q_east);
	g_list_free(destroy_w);
	g_list_free(destroy_e);
	empty_graph(graph_east);
	pg_graph_destroy(graph_east);

	if (pg_graph_explore_ptr(graph_west, NULL, error) < 0)
		return -1;
	return 0;
}

int pg_graph_dot(struct pg_graph *graph, FILE *fd,
		 struct pg_error **error)
{
	return pg_brick_dot(get_any_brick(graph), fd, error);
}

void pg_graph_empty(struct pg_graph *graph)
{
	empty_graph(graph);
}
