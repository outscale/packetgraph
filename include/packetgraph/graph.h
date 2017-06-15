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

#ifndef _PG_GRAPH_H
#define _PG_GRAPH_H

#include <stdio.h>
#include <packetgraph/errors.h>

struct pg_brick;
struct pg_graph;

/**
 * Allocate and init an empty graph.
 *
 * A graph is like a brick container: you can add bricks to it (push),
 * you can remove bricks (pop) or directly destroy bricks inside graph.
 * Graph are useful to manipulate bricks by their name and not having to store
 * all bricks by ourself.
 *
 * An important thing to know is that graph (and bricks) are not thread safe:
 * - You must not call pg_graph_poll on the same graph in parallel.
 * - You must not add the same brick in two different graph and use them in
 *   parallel.
 *
 * @param	graph graph's name
 * @param	explore if set, explore graph from this brick.
 *		See pg_graph_explore for more information.
 *		Create a empty graph if set to NULL.
 * @return      new allocated and initialized graph, NULL on fail.
 */
PG_WARN_UNUSED
struct pg_graph *pg_graph_new(const char *name, struct pg_brick *explore,
			      struct pg_error **error);

/**
 * Get graph's name.
 *
 * @param	graph from which we get name
 * @return	name graph's name
 */
const char *pg_graph_name(struct pg_graph *graph);

/**
 * Destroy and free a graph with all bricks inside.
 *
 * @param	graph graph which is going to be destroy
 */
void pg_graph_destroy(struct pg_graph *graph);

/**
 * Explore graph starting from a brick.
 *
 * All bricks linked to the chosen brick will be added to the graph.
 * Note that bricks should be used by only one graph and all bricks inside a
 * graph should have a different name.
 * You can call this function on an altered graph to re-explore it.
 * If you do so, bricks which are not part of the graph anymore will be removed
 * and new linked bricks will be added to graph.
 *
 * @param	graph graph where explored bricks goes
 * @param	brick_name brick name from where to start adding bricks.
		Set to NULL and graph will any brick to start exploring.
 * @param	error is set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_graph_explore(struct pg_graph *graph,
		     const char *brick_name,
		     struct pg_error **error);

/**
 * Same as pg_graph_explore() but provides a brick pointer.
 *
 * @param	graph graph where explored bricks goes
 * @param	brick brick from where to start adding bricks.
		Set to NULL and graph will any brick to start exploring.
 * @param	error is set in case of an error
 * @return	0 on success, -1 on error
 */

int pg_graph_explore_ptr(struct pg_graph *graph,
			 struct pg_brick *brick,
			 struct pg_error **error);

/**
* Poll all pollable bricks of the graph.
* Stops on first poll error.
*
* @param	graph graph to poll
* @param	error is set in case of an error
* @return	0 on success, -1 on error
*/
int pg_graph_poll(struct pg_graph *graph, struct pg_error **error);

/**
 * Sanity check for a graph.
 *
 * This test permits to ensure that a graph is consistant and don't contains
 * isolated parts.
 *
 * @param	graph graph to be used for sanity check
 * @param	error is set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_graph_sanity(struct pg_graph *graph, struct pg_error **error);

/**
 * Get a brick from a graph by it's name.
 *
 * @param	graph where to get brick
 * @param	brick_name name of the brick to get
 * @return	pointer to brick, NULL if not found
 */
struct pg_brick *pg_graph_get(struct pg_graph *graph, const char *brick_name);

/**
 * Remove a brick from it's graph giving it's name and return it.
 * Graph may not pass sanity check as the brick stay untouched.
 *
 * @param	graph graph where to remove a brick
 * @param	brick_name name of the brick to remove
 * @return	pointer to brick, NULL if not found
 */
struct pg_brick *pg_graph_unsafe_pop(struct pg_graph *graph,
				     const char *brick_name);

/**
 * Remove all bricks from a graph without destroying them
 *
 * @param	graph graph where to remove all bricks
 */
void pg_graph_empty(struct pg_graph *graph);

/**
 * Add a single brick to graph.
 *
 * @param	graph graph where to add a brick
 * @param	brick brick to add to graph
 * @param	error is set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_graph_push(struct pg_graph *graph,
		  struct pg_brick *brick,
		  struct pg_error **error);

/**
 * Count the number of bricks in a graph.
 *
 * @param	graph graph
 * @return	number of bricks in the graph
 */
int pg_graph_count(struct pg_graph *graph);

/**
 * Destroy a brick of a graph giving it's name.
 *
 * @param	graph graph where to destroy a brick
 * @param	brick_name name of the brick to destroy
 * @param	error is set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_graph_brick_destroy(struct pg_graph *graph,
			   const char *brick_name,
			   struct pg_error **error);

/**
 * Split a graph in two separated graph.
 * Communication between graph is done using two queue brick.
 *
 * Before: [A]----[B]----[C]----[D]
 *
 * => split (B, C)
 *
 * After: [A]----[B]----[Queue1] ~ [Queue2]----[C]----[D]
 *
 * @param       graph graph to split. This graph will contain the west part of
 *		the graph after split.
 * @param	east_graph_name name of the newly created graph containing the
 *		east part of the graph after split.
 * @param	west_brick_name name of the brick where to split, this brick
 *		will be part of west_graph.
 * @param	east_brick_name name of the brick where to split, this brick
 *		will be part of	east_graph.
 * @param	west_queue_name name of the west queue (with default queue size)
 * @param	east_queue_name name of the east queue (with default queue size)
 * @param	error is set in case of an error
 * @return	pointer to newly created graph corresponding to the east part of
 *		the graph. NULL on error.
 */
struct pg_graph *pg_graph_split(struct pg_graph *graph,
				const char *east_graph_name,
				const char *west_brick_name,
				const char *east_brick_name,
				const char *west_queue_name,
				const char *east_queue_name,
				struct pg_error **error);

/**
 * Merge two graphs into one.
 * Remove all queues between two graph and merge the whole graph into one.
 *
 * @param	graph_west graph who will contain the whole graph
 * @param	graph_east graph who will be merged into graph_west.
 *		this graph will be free and should not be used anymore.
 * @param	error is set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_graph_merge(struct pg_graph *graph1,
		   struct pg_graph *graph2,
		   struct pg_error **error);

/**
 * Write a dot (graphviz) graph to a file descriptor from a graph.
 * If the graph is splitted, you will see more than one graph.
 *
 * @param	graph graph structure used to build dot graph
 * @param	fd file descriptor where to write the graph description
 */
void pg_graph_dot(struct pg_graph *graph, FILE *fd);

#endif /* _PG_GRAPH_H */
