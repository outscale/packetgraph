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
inline enum pg_side pg_flip_side(enum pg_side side)
{
	return (side ^ 1);
}

static void assert_brick_callback(struct pg_brick *brick)
{
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

		if (brick->type == PG_MULTIPOLE) {
			side->nb = 0;
			g_assert(side->max);
			side->edges = g_new0(struct pg_brick_edge, side->max);
		} else {
			side->edge.link = NULL;
			side->edge.pair_index = 0;
		}
	}
}

/**
 * This should be called by the init constructor so brick_new can alloc arrays
 *
 * @param	brick the struct pg_brick we are constructing
 * @param	west_edges the size of the west array
 * @param	east_edges the size of the east array
 */
static void pg_brick_set_max_edges(struct pg_brick *brick,
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

static int check_side_max(struct pg_brick_config *config,
			  struct pg_error **errp)
{
	int overedge = (config->west_max >= UINT16_MAX ||
			config->east_max >= UINT16_MAX);

	if (config->type == PG_MULTIPOLE && overedge) {

		enum pg_side faulte = config->west_max >= UINT16_MAX ?
			WEST_SIDE : EAST_SIDE;

		*errp = pg_error_new(
			"A '%s' cannot have more than %d edge on %s",
			pg_brick_type_to_string(config->type),
			UINT16_MAX, pg_side_to_string(faulte));
		return -1;

	} else if ((config->type == PG_MONOPOLE ||
		    config->type == PG_DIPOLE) &&
		   (config->west_max > 1 ||  config->east_max > 1)) {
		*errp = pg_error_new(
			"A '%s' cannot have more than one neibour per side",
			pg_brick_type_to_string(config->type));
		return -1;
	}
	return 0;
}

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
	brick->type = config->type;

	zero_brick_counters(brick);

	if (check_side_max(config, errp) < 0)
		goto fail_exit;

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

	if (brick->type == PG_MULTIPOLE) {
		for (i = 0; i < MAX_SIDE; i++)
			g_free(brick->sides[i].edges);
	}

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
		return -1;
	}

	if (!brick->ops || !brick->ops->reset)
		return -1;

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
	struct pg_brick_side *side;

	if (!brick) {
		*errp = pg_error_new("brick is NULL");
		return 0;
	}

	if (brick->type == PG_MULTIPOLE) {
		for (i = 0; i < MAX_SIDE; i++)
			count += count_side(&brick->sides[i],
					    target);
	} else if (brick->type == PG_DIPOLE) {
		for (i = 0; i < MAX_SIDE; i++) {
			side = &brick->sides[i];
			if (side->edge.link && side->edge.link == target)
				++count;
		}
	} else if (brick->type == PG_MONOPOLE) {
		side = &brick->side;
		if (side->edge.link && side->edge.link == target)
			++count;
	} else {
		return -1;
	}

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



struct pg_brick_edge *pg_brick_get_edge(struct pg_brick *brick,
					enum pg_side side,
					uint32_t edge)
{
	switch (brick->type) {
	case PG_MULTIPOLE:
		return &brick->sides[side].edges[edge];
	case PG_DIPOLE:
		return &brick->sides[side].edge;
	case PG_MONOPOLE:
		return &brick->side.edge;
	}
	return NULL;
}

/**
 * link @from to @to, on @side
 */
static uint16_t insert_link(struct pg_brick *from,
			    struct pg_brick *to,
			    enum pg_side side)
{
	struct pg_brick_side *s;

	switch (from->type) {
	case PG_MULTIPOLE:
		s = &from->sides[side];
		g_assert(s->edges);

		for (uint16_t i = 0; i < s->max; i++) {
			if (!s->edges[i].link) {
				s->nb++;
				pg_brick_incref(to);
				s->edges[i].link = to;
				return i;
			}
		}

		/* This should never happen */
		g_assert(0);
		return 0;
	case PG_DIPOLE:
		from->sides[side].edge.link = to;
		from->sides[side].nb++;
		pg_brick_incref(to);
		return 0;
	case PG_MONOPOLE:
		from->side.nb++;
		pg_brick_incref(to);
		from->side.edge.link = to;
		return 0;
	}
	return 0;
}

static int is_place_available(struct pg_brick *brick, enum pg_side side)
{
	switch (brick->type) {
	case PG_MONOPOLE:
		return (brick->side.nb < brick->side.max);
	case PG_DIPOLE:
	case PG_MULTIPOLE:
		return (brick->sides[side].nb < brick->sides[side].max);
	}
	return 0;
}

int pg_brick_link(struct pg_brick *west,
		  struct pg_brick *east,
		  struct pg_error **errp)
{
	uint16_t west_index, east_index;

	if (!is_brick_valid(east) || !is_brick_valid(west)) {
		*errp = pg_error_new("Node is not valid");
		return -1;
	}

	if (west == east) {
		*errp = pg_error_new("Can not link a brick to herself");
		return -1;
	}
	/* check if each sides have places */
	if (!is_place_available(east, WEST_SIDE)) {
		*errp = pg_error_new("%s: Side full", east->name);
		return -1;
	}
	if (!is_place_available(west, EAST_SIDE)) {
		*errp = pg_error_new("%s: Side full", west->name);
		return -1;
	}

	/* insert and get pair index */
	east_index = insert_link(east, west, WEST_SIDE);
	if (east->ops->link_notify)
		east->ops->link_notify(east, WEST_SIDE, east_index);

	west_index = insert_link(west, east, EAST_SIDE);
	if (west->ops->link_notify)
		west->ops->link_notify(west, EAST_SIDE, west_index);

	/* finish the pairing of the edge */
	pg_brick_get_edge(east, WEST_SIDE, east_index)->pair_index = west_index;
	pg_brick_get_edge(west, EAST_SIDE, west_index)->pair_index = east_index;

	return 0;
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
		if (ret < 0)
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

static struct pg_brick_side *get_side(struct pg_brick *brick, enum pg_side side)
{
	if (brick->type == PG_MONOPOLE)
		return &brick->side;
	return &brick->sides[side];
}

static void do_unlink(struct pg_brick *brick, enum pg_side side, uint16_t index,
		      struct pg_error **errp)
{
	struct pg_brick_edge *edge = pg_brick_get_edge(brick, side, index);
	struct pg_brick_side *pair_side;
	struct pg_brick_edge *pair_edge;

	if (brick->type == PG_MONOPOLE) {
		g_assert(brick->ops->get_side);
		side = brick->ops->get_side(brick);
	}
	if (!edge->link)
		return;

	pair_side = get_side(edge->link, pg_flip_side(side));
	pair_edge = pg_brick_get_edge(edge->link,
				      pg_flip_side(side),
				      edge->pair_index);

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

static void brick_generic_unlink_multipole(struct pg_brick *brick,
					   enum pg_side side,
					   struct pg_error **errp)
{
	uint16_t i;

	for (i = 0; i < brick->sides[side].max; i++) {
		do_unlink(brick, side, i, errp);

		if (pg_error_is_set(errp))
			return;
	}
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

	for (i = 0; i < MAX_SIDE; i++) {
		switch (brick->type) {
		case PG_MULTIPOLE:
			brick_generic_unlink_multipole(brick, i, errp);
			break;
		case PG_DIPOLE:
			do_unlink(brick, i, 0, errp);
			break;
		case PG_MONOPOLE:
			do_unlink(brick, 0, 0, errp);
			/* there's only one side, so we can return now */
			return;
		}
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

	if (!brick->ops->burst_get) {
		*errp = pg_error_new("No burst_get callback");
		return NULL;
	}

	return brick->ops->burst_get(brick, WEST_SIDE, pkts_mask);
}

struct rte_mbuf **pg_brick_east_burst_get(struct pg_brick *brick,
					  uint64_t *pkts_mask,
					  struct pg_error **errp)
{
	if (!brick) {
		*errp = pg_error_new("NULL brick");
		return NULL;
	}

	if (!brick->ops->burst_get) {
		*errp = pg_error_new("No burst_get callback");
		return NULL;
	}

	return brick->ops->burst_get(brick, EAST_SIDE, pkts_mask);
}

int pg_brick_side_forward(struct pg_brick_side *brick_side,
			  enum pg_side from, struct rte_mbuf **pkts,
			  uint16_t nb, uint64_t pkts_mask,
			  struct pg_error **errp)
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
	switch (brick->type) {
	case PG_MULTIPOLE:
	case PG_DIPOLE:
		return rte_atomic64_read(&brick->sides[side].packet_count);
	case PG_MONOPOLE:
		return rte_atomic64_read(&brick->side.packet_count);
	}
	return 0;
}

const char *pg_brick_name(struct pg_brick *brick)
{
	return brick->name;
}

const char *pg_brick_type(struct pg_brick *brick)
{
	return brick->ops->name;
}

uint32_t pg_side_get_max(struct pg_brick *brick, enum pg_side side)
{
	return brick->sides[side].max;
}
