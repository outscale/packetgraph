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

/* This file contains all the common infrastructure from brick management and
 * the public brick API function implementations.
 */

#include <string.h>
#include <glib.h>
#include <packetgraph/brick.h>
#include <packetgraph/utils/bitmask.h>

/* All registred bricks. */
GList *pg_all_bricks;

/* return the oposite side */
enum pg_side pg_flip_side(enum pg_side side)
{
	if (side == WEST_SIDE)
		return EAST_SIDE;
	else if (side == EAST_SIDE)
		return WEST_SIDE;
	g_assert(0);
	return 0;
}

static void assert_brick_callback(struct pg_brick *brick)
{
	enum pg_side i;

	/* the init constructor should set these */
	for (i = 0; i < MAX_SIDE; i++)
		g_assert(brick->sides[i].max);
	/* assert that the minimum of functions pointers are filled */
	g_assert(brick->burst);
	g_assert(brick->ops);
}

/**
 * This function is used to allocate the edges arrays
 *
 * @brick: the struct pg_brick we are initializing
 */
static void alloc_edges(struct pg_brick *brick)
{
	enum pg_side i;

	for (i = 0; i < MAX_SIDE; i++) {
		struct pg_brick_side *side = &brick->sides[i];

		side->nb = 0;
		g_assert(side->max);
		side->edges = g_new0(struct pg_brick_edge, side->max);
	}
}

/**
 * This should be called by the init constructor so brick_new can alloc arrays
 *
 * @param	brick the struct pg_brick we are constructing
 * @param	west_edges the size of the west array
 * @param	east_edges the size of the east array
 */
void pg_brick_set_max_edges(struct pg_brick *brick,
			    uint16_t west_edges,
			    uint16_t east_edges)
{
	brick->sides[WEST_SIDE].max = west_edges;
	brick->sides[EAST_SIDE].max = east_edges;
}

static void zero_brick_counters(struct pg_brick *brick)
{
	enum pg_side i;

	for (i = 0; i < MAX_SIDE; ++i)
		rte_atomic64_set(&brick->sides[i].packet_count, 0);
}

/* Convenient macro to get a pointer to brick ops */
#define pg_brick_get(it) ((struct pg_brick_ops *)(it->data))

/**
 * This function instantiates a brick by its brick_ops name
 *
 * @param	name the brick_ops name to lookup
 * @param	config the brick configuration
 * @param	errp a return pointer for an error message
 * @return	the instantiated brick
 */
struct pg_brick *pg_brick_new(const char *name,
			      struct pg_brick_config *config,
			      struct pg_error **errp)
{
	struct pg_brick *brick;
	size_t ret;
	GList *it;

	if (!name) {
		*errp = pg_error_new("Brick name not set");
		return NULL;
	}

	for (it = pg_all_bricks; it; it = it->next) {
		/* the east brick_ops is found */
		if (!g_strcmp0(name, pg_brick_get(it)->name))
			break;
	}

	/* correct brick_ops not found -> fail */
	if (it == NULL) {
		*errp = pg_error_new("Brick '%s' not found", name);
		return NULL;
	}

	if (pg_brick_get(it)->state_size <= 0) {
		*errp = pg_error_new("Brick state size is not positive");
		return NULL;
	}

	/* The brick struct is be the first member of the state. */
	brick = g_malloc0(pg_brick_get(it)->state_size);
	brick->ops = pg_brick_get(it);
	brick->refcount = 1;

	zero_brick_counters(brick);

	if (config->west_max >= UINT16_MAX) {
		*errp = pg_error_new("Supports UINT16_MAX west neigbourghs");
		goto fail_exit;
	}

	if (config->east_max >= UINT16_MAX) {
		*errp = pg_error_new("Supports UINT16_MAX east neigbourghs");
		goto fail_exit;
	}

	pg_brick_set_max_edges(brick, config->west_max, config->east_max);
	brick->name = g_strdup(config->name);

	ret = brick->ops->init(brick, config, errp);

	if (!ret) {
		if (!pg_error_is_set(errp))
			*errp = pg_error_new("Failed to init '%s' brick", name);
		goto fail_exit;
	}

	assert_brick_callback(brick);
	/* TODO: register the brick */
	alloc_edges(brick);
	return brick;

fail_exit:
	g_free(brick->name);
	g_free(brick);
	return NULL;
}

#undef brick_get

void pg_brick_incref(struct pg_brick *brick)
{
	brick->refcount++;
}

static int is_brick_valid(struct pg_brick *brick)
{
	if (!brick)
		return 0;

	if (!brick->ops)
		return 0;

	return 1;
}

void pg_brick_destroy(struct pg_brick *brick)
{
	struct pg_error *errp = NULL;

	pg_brick_unlink(brick, &errp);
	pg_brick_decref(brick, &errp);
}

/**
 * This function decref a brick and it's private data
 *
 * @param	brick the brick to decref
 * @param	errp a return pointer for an error message
 * @return	the brick if refcount >= or NULL if it has been destroyed
 */
struct pg_brick *pg_brick_decref(struct pg_brick *brick, struct pg_error **errp)
{
	enum pg_side i;

	if (!brick) {
		*errp = pg_error_new("NULL brick");
		return NULL;
	}

	brick->refcount--;

	if (brick->refcount > 0)
		return brick;

	if (!is_brick_valid(brick))
		return brick;

	if (brick->ops->destroy)
		brick->ops->destroy(brick, errp);

	for (i = 0; i < MAX_SIDE; i++)
		g_free(brick->sides[i].edges);

	g_free(brick->name);
	/* The brick struct is be the first member of the state. */
	g_free(brick);
	return NULL;
}

/**
 * This reset a brick
 *
 * @param	brick the brick we are working with
 * @param	errp a return pointer for an error message
 * @return	1 on success, 0 on error
 */
int pg_brick_reset(struct pg_brick *brick, struct pg_error **errp)
{
	if (!brick) {
		*errp = pg_error_new("brick is NULL");
		return 0;
	}

	if (!brick->ops || !brick->ops->reset)
		return 0;

	return brick->ops->reset(brick, errp);
}

/**
 * This function will be used in unit test to check a brick refcount
 *
 * @param	brick the brick to get the refcount from
 * @return	the refcount
 */
int64_t pg_brick_refcount(struct pg_brick *brick)
{
	g_assert(brick->refcount > 0);
	return brick->refcount;
}

static uint16_t count_side(struct pg_brick_side *side,
			   struct pg_brick *target)
{
	uint16_t j, count = 0;

	for (j = 0; j < side->max; j++)
		if (side->edges[j].link && side->edges[j].link == target)
			count++;
	return count;
}

/**
 * This function count the number of links to another brick
 *
 * @param	brick the link to inspect while counting the links
 * @param	target the brick to look for in the edge arrays
 * @param	errp a return pointer for an error message
 * @return	the total number of link from the brick to the target
 */
uint32_t pg_brick_links_count_get(struct pg_brick *brick,
				  struct pg_brick *target,
				  struct pg_error **errp)
{
	uint32_t count = 0;
	enum pg_side i;

	if (!brick) {
		*errp = pg_error_new("brick is NULL");
		return 0;
	}

	for (i = 0; i < MAX_SIDE; i++)
		count += count_side(&brick->sides[i], target);

	return count;
}

/**
 * This function unlinks all the link from and to this brick
 *
 * @param	brick the brick brick to unlink
 * @param	errp a return pointer for an error message
 */
void pg_brick_unlink(struct pg_brick *brick, struct pg_error **errp)
{
	if (!is_brick_valid(brick)) {
		*errp = pg_error_new("Node is not valid");
		return;
	}

	if (!brick->ops->unlink) {
		*errp = pg_error_new("Node unlink callback not set");
		return;
	}

	brick->ops->unlink(brick, errp);
}

/**
 * This function insert a brick into the first empty edge of an edge array
 *
 * @param	side the side to insert into
 * @param	brick the brick to link to
 * @return	the insertion index in the array
 *
 */
static uint16_t insert_link(struct pg_brick_side *side,
			    struct pg_brick *brick)
{
	uint16_t i;

	g_assert(side->edges);

	for (i = 0; i < side->max; i++)
		if (!side->edges[i].link) {
			side->nb++;
			pg_brick_incref(brick);
			side->edges[i].link = brick;
			return i;
		}

	g_assert(0);
	return 0;
}


int pg_brick_link(struct pg_brick *west,
		  struct pg_brick *east,
		  struct pg_error **errp)
{
	uint16_t west_index, east_index;

	if (!is_brick_valid(east) || !is_brick_valid(west)) {
		*errp = pg_error_new("Node is not valid");
		return 0;
	}

	if (west == east) {
		*errp = pg_error_new("Can not link a brick to herself");
		return 0;
	}
	if (east->sides[WEST_SIDE].nb == east->sides[WEST_SIDE].max) {
		*errp = pg_error_new("%s: Side full", east->name);
		return 0;
	}
	if (west->sides[EAST_SIDE].nb == west->sides[EAST_SIDE].max) {
		*errp = pg_error_new("%s: Side full", west->name);
		return 0;
	}

	east_index = insert_link(&east->sides[WEST_SIDE], west);
	west_index = insert_link(&west->sides[EAST_SIDE], east);

	/* finish the pairing of the edge */
	east->sides[WEST_SIDE].edges[east_index].pair_index = west_index;
	west->sides[EAST_SIDE].edges[west_index].pair_index = east_index;

	return 1;
}

int pg_brick_chained_links_int(struct pg_error **errp,
			       struct pg_brick *west, ...)
{
	va_list ap;
	struct pg_brick *east;
	int ret = 1;

	va_start(ap, west);
	while ((east = va_arg(ap, struct pg_brick *)) != NULL) {
		ret = pg_brick_link(west, east, errp);
		if (!ret)
			goto exit;
		west = east;
	}
exit:
	va_end(ap);
	return ret;
}

static void reset_edge(struct pg_brick_edge *edge)
{
	edge->link = NULL;
	edge->pair_index = 0;
}

/* Optionaly notify the edge we are unlinking with of the unlink event
 *
 * @edge: the edge we are unlinking with
 */
static void unlink_notify(struct pg_brick_edge *edge, enum pg_side array_side,
			  struct pg_error **errp)
{
	struct pg_brick *brick = edge->link;

	if (!brick)
		return;

	if (!brick->ops || !brick->ops->unlink_notify)
		return;

	brick->ops->unlink_notify(brick,
				  pg_flip_side(array_side),
				  edge->pair_index, errp);
}

static void do_unlink(struct pg_brick *brick, enum pg_side side, uint16_t index,
		      struct pg_error **errp)
{
	struct pg_brick_edge *edge = &brick->sides[side].edges[index];
	struct pg_brick_edge *pair_edge;
	struct pg_brick_side *pair_side;

	if (!edge->link)
		return;

	pair_side = &edge->link->sides[pg_flip_side(side)];
	pair_edge = &pair_side->edges[edge->pair_index];

	unlink_notify(edge, side, errp);
	if (pg_error_is_set(errp))
		return;

	pg_brick_decref(brick, errp);

	if (pg_error_is_set(errp))
		return;

	pg_brick_decref(edge->link, errp);

	if (pg_error_is_set(errp))
		return;

	reset_edge(pair_edge);
	reset_edge(edge);

	brick->sides[side].nb--;
	pair_side->nb--;
}

/**
 * This function unlinks all the link from and to this brick
 *
 * @param	brick the brick brick to unlink
 * @param	errp a return pointer for an error message
 */
void pg_brick_generic_unlink(struct pg_brick *brick, struct pg_error **errp)
{
	enum pg_side i;
	uint16_t j;

	for (i = 0; i < MAX_SIDE; i++)
		for (j = 0; j < brick->sides[i].max; j++) {
			do_unlink(brick, i, j, errp);

			if (pg_error_is_set(errp))
				return;
		}
}

/**
 * Data flow public API functions
 *
 * These inlined functions are the fast path so no extra check is done to assert
 * pointer validity.
 * These check are already done at the end of the brick init public API
 * function.
 */

inline int pg_brick_burst(struct pg_brick *brick, enum pg_side from,
			  uint16_t edge_index, struct rte_mbuf **pkts,
			  uint16_t nb, uint64_t pkts_mask,
			  struct pg_error **errp)
{
	/* @side is the opposite side of the direction on which
	 * we send the packets, so we flip it */
	rte_atomic64_add(&brick->sides[pg_flip_side(from)].packet_count,
			 pg_mask_count(pkts_mask));
	return brick->burst(brick, from, edge_index, pkts, nb, pkts_mask, errp);
}

inline int pg_brick_burst_to_east(struct pg_brick *brick, uint16_t edge_index,
				  struct rte_mbuf **pkts, uint16_t nb,
				  uint64_t pkts_mask, struct pg_error **errp)
{
	return pg_brick_burst(brick, WEST_SIDE, edge_index,
			      pkts, nb, pkts_mask, errp);
}

inline int pg_brick_burst_to_west(struct pg_brick *brick, uint16_t edge_index,
				  struct rte_mbuf **pkts, uint16_t nb,
				  uint64_t pkts_mask, struct pg_error **errp)
{
	return pg_brick_burst(brick, EAST_SIDE, edge_index,
			      pkts, nb, pkts_mask, errp);
}

int pg_brick_poll(struct pg_brick *brick,
		  uint16_t *count, struct pg_error **errp)
{
	if (!brick) {
		*errp = pg_error_new("Brick is NULL");
		return 0;
	}

	if (!brick->poll) {
		*errp = pg_error_new("No poll callback");
		return 0;
	}

	return brick->poll(brick, count, errp);
}

/* These functions are are for automated testing purpose */
struct rte_mbuf **pg_brick_west_burst_get(struct pg_brick *brick,
					  uint64_t *pkts_mask,
					  struct pg_error **errp)
{
	if (!brick) {
		*errp = pg_error_new("NULL brick");
		return NULL;
	}

	if (!brick->burst_get) {
		*errp = pg_error_new("No burst_get callback");
		return NULL;
	}

	return brick->burst_get(brick, WEST_SIDE, pkts_mask);
}

struct rte_mbuf **pg_brick_east_burst_get(struct pg_brick *brick,
					  uint64_t *pkts_mask,
					  struct pg_error **errp)
{
	if (!brick) {
		*errp = pg_error_new("NULL brick");
		return NULL;
	}

	if (!brick->burst_get) {
		*errp = pg_error_new("No burst_get callback");
		return NULL;
	}

	return brick->burst_get(brick, EAST_SIDE, pkts_mask);
}

int pg_brick_side_forward(struct pg_brick_side *brick_side, enum pg_side from,
			  struct rte_mbuf **pkts, uint16_t nb,
			  uint64_t pkts_mask, struct pg_error **errp)
{
	int ret = 1;
	uint16_t i;

	for (i = 0; i < brick_side->max; i++) {
		if (brick_side->edges[i].link)
			ret = pg_brick_burst(brick_side->edges[i].link, from,
					     brick_side->edges[i].pair_index,
					     pkts, nb, pkts_mask, errp);
		if (unlikely(!ret))
			return 0;
	}

	return 1;
}

uint64_t pg_brick_pkts_count_get(struct pg_brick *brick, enum pg_side side)
{
	if (!brick)
		return 0;
	return rte_atomic64_read(&brick->sides[side].packet_count);
}

const char *pg_brick_name(struct pg_brick *brick)
{
	return brick->name;
}

const char *pg_brick_type(struct pg_brick *brick)
{
	return brick->ops->name;
}
