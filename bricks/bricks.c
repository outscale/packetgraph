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

#include "bricks/brick.h"

/* return the oposite side */
inline enum side flip_side(enum side side)
{
	if (side == WEST_SIDE)
		return EAST_SIDE;
	else if (side == EAST_SIDE)
		return WEST_SIDE;
	g_assert(0);
	return 0;
}

static void assert_brick_callback(struct brick *brick)
{
	enum side i;

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
 * @brick: the struct brick we are initializing
 */
static void alloc_edges(struct brick *brick)
{
	enum side i;

	for (i = 0; i < MAX_SIDE; i++) {
		struct brick_side *side = &brick->sides[i];

		side->nb = 0;
		g_assert(side->max);
		side->edges = g_new0(struct brick_edge, side->max);
	}
}

/**
 * This should be called by the init constructor so brick_new can alloc arrays
 *
 * @param	brick the struct brick we are constructing
 * @param	west_edges the size of the west array
 * @param	east_edges the size of the east array
 */
void brick_set_max_edges(struct brick *brick,
			   uint16_t west_edges, uint16_t east_edges)
{
	brick->sides[WEST_SIDE].max = west_edges;
	brick->sides[EAST_SIDE].max = east_edges;
}

/**
 * This function instantiates a brick by its brick_ops name
 *
 * @param	name the brick_ops name to lookup
 * @param	config the brick configuration
 * @return	the instantiated brick
 */
struct brick *brick_new(const char *name, struct brick_config *config)
{
	struct brick_ops **bricks;
	struct brick *brick;
	size_t num, i, ret;

	if (!name)
		return NULL;

	bricks = autodata_get(brick, &num);

	for (i = 0; i < num; i++) {
		/* the east brick_ops is found */
		if (!g_strcmp0(name, bricks[i]->name))
			break;
	}

	/* correct brick_ops not found -> fail */
	if (i == num)
		return NULL;

	if (bricks[i]->state_size <= 0)
		return NULL;

	/* The brick struct is be the first member of the state. */
	brick = g_malloc0(bricks[i]->state_size);
	brick->ops = bricks[i];
	brick->refcount = 1;

	g_assert(config->west_max <= UINT16_MAX);
	g_assert(config->east_max <= UINT16_MAX);

	brick_set_max_edges(brick, config->west_max, config->east_max);
	brick->name = g_strdup(config->name);

	ret = brick->ops->init(brick, config);

	if (!ret)
		goto fail_exit;

	assert_brick_callback(brick);
	/* TODO: register the brick */
	alloc_edges(brick);
	return brick;

fail_exit:
	g_free(brick->name);
	g_free(brick);
	return NULL;
}

void brick_incref(struct brick *brick)
{
	brick->refcount++;
}

static int is_brick_valid(struct brick *brick)
{
	if (!brick)
		return 0;

	if (!brick->ops)
		return 0;

	return 1;
}

/**
 * This function decref a brick and it's private data
 *
 * @param	brick the brick to decref
 * @return	the brick if refcount >= or NULL if it has been destroyed
 */
struct brick *brick_decref(struct brick *brick)
{
	enum side i;

	if (!brick)
		return NULL;

	brick->refcount--;

	if (brick->refcount > 0)
		return brick;

	if (!is_brick_valid(brick))
		return brick;

	if (brick->ops->destroy)
		brick->ops->destroy(brick);

	for (i = 0; i < MAX_SIDE; i++)
		g_free(brick->sides[i].edges);

	g_free(brick->name);
	/* The brick struct is be the first member of the state. */
	g_free(brick);
	return NULL;
}

/**
 * This function will be used in unit test to check a brick refcount
 *
 * @param	brick the brick to get the refcount from
 * @return	the refcount
 */
int64_t brick_refcount(struct brick *brick)
{
	g_assert(brick->refcount > 0);
	return brick->refcount;
}

static uint16_t count_side(struct brick_side *side, struct brick *target)
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
 * @return	the total number of link from the brick to the target
 */
uint32_t brick_links_count_get(struct brick *brick, struct brick *target)
{
	uint32_t count = 0;
	enum side i;

	if (!brick)
		return 0;

	for (i = 0; i < MAX_SIDE; i++)
		count += count_side(&brick->sides[i], target);

	return count;
}

/**
 * This function add a west link from target to brick
 *
 * This function will also add a east link from brick to target.
 * It establish bidirectionals links.
 *
 * @param	target the target of the link operation
 * @param	brick the brick to link to the target
 * @return	1 on success, 0 on error
 */
int brick_west_link(struct brick *target, struct brick *brick)
{
	if (!is_brick_valid(brick))
		return 0;

	if (!brick->ops->west_link)
		return 0;

	return brick->ops->west_link(target, brick);
}

/**
 * This function add a east link from target to brick
 *
 * This function will also add a west link from brick to target.
 * It establish bidirectionals links.
 *
 * @param	target the target of the link operation
 * @param	brick the brick to link to the target
 * @return	1 on success, 0 on error
 */
int brick_east_link(struct brick *target, struct brick *brick)
{
	if (!is_brick_valid(brick))
		return 0;

	if (!brick->ops->east_link)
		return 0;

	return brick->ops->east_link(target, brick);
}

/**
 * This function unlinks all the link from and to this brick
 *
 * @param	brick the brick brick to unlink
 */
void brick_unlink(struct brick *brick)
{
	if (!is_brick_valid(brick))
		return;

	if (!brick->ops->unlink)
		return;

	brick->ops->unlink(brick);
}

/**
 * This function insert a brick into the first empty edge of an edge array
 *
 * @param	side the side to insert into
 * @param	brick the brick to link to
 * @return	the insertion index in the array
 *
 */
static uint16_t insert_link(struct brick_side *side, struct brick *brick)
{
	uint16_t i;

	g_assert(side->edges);

	for (i = 0; i < side->max; i++)
		if (!side->edges[i].link) {
			side->nb++;
			brick_incref(brick);
			side->edges[i].link = brick;
			return i;
		}

	g_assert(0);
}

/**
 * Default implementation is made to fill the west links of brick_ops
 *
 * @param	target the target of the link operation
 * @param	brick the brick to link to the target
 * @return	1 on success, 0 on error
 */
int brick_generic_west_link(struct brick *target, struct brick *brick)
{
	uint16_t target_index, brick_index;
	enum side i;

	for (i = 0; i < MAX_SIDE; i++) {
		struct brick_side *side = &target->sides[i];

		if (side->nb == side->max)
			return 0;
	}

	target_index = insert_link(&target->sides[WEST_SIDE], brick);
	brick_index = insert_link(&brick->sides[EAST_SIDE], target);

	/* finish the pairing of the edge */
	brick->sides[EAST_SIDE].edges[brick_index].pair_index = target_index;
	target->sides[WEST_SIDE].edges[target_index].pair_index = brick_index;

	return 1;
}

/**
 * Default implementation is made to fill the east links of brick_ops
 *
 * @param	target the target of the link operation
 * @param	brick the brick to link to the target
 * @return	1 on success, 0 on error
 */
int brick_generic_east_link(struct brick *target, struct brick *brick)
{
	return brick_generic_west_link(brick, target);
}

static void reset_edge(struct brick_edge *edge)
{
	edge->link = NULL;
	edge->pair_index = 0;
}

static void do_unlink(struct brick *brick, enum side side, uint16_t index)
{
	struct brick_edge *edge = &brick->sides[side].edges[index];
	struct brick_edge *pair_edge;

	if (!edge->link)
		return;

	pair_edge = &edge->link->sides[flip_side(side)].edges[edge->pair_index];

	brick_decref(brick);
	brick_decref(edge->link);
	reset_edge(pair_edge);
	reset_edge(edge);

	brick->sides[side].nb--;
}

/**
 * This function unlinks all the link from and to this brick
 *
 * @param	brick the brick brick to unlink
 */
void brick_generic_unlink(struct brick *brick)
{
	enum side i;
	uint16_t j;

	for (i = 0; i < MAX_SIDE; i++)
		for (j = 0; j < brick->sides[i].max; j++)
			do_unlink(brick, i, j);
}

/**
 * Data flow public API functions
 *
 * These inlined functions are the fast path so no extra check is done to assert
 * pointer validity.
 * These check are already done at the end of the brick init public API
 * function.
 */

inline void brick_burst(struct brick *brick, enum side side,
			struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask)
{
	brick->burst(brick, side, pkts, nb, pkts_mask);

}

inline void brick_burst_to_east(struct brick *brick, struct rte_mbuf **pkts,
				uint16_t nb, uint64_t pkts_mask)
{
	brick_burst(brick, WEST_SIDE, pkts, nb, pkts_mask);
}

inline void brick_burst_to_west(struct brick *brick, struct rte_mbuf **pkts,
				uint16_t nb, uint64_t pkts_mask)
{
	brick_burst(brick, EAST_SIDE, pkts, nb, pkts_mask);
}

/* These functions are are for automated testing purpose */
struct rte_mbuf **brick_west_burst_get(struct brick *brick,
				       uint64_t *pkts_mask)
{
	if (!brick)
		return NULL;

	if (!brick->burst_get)
		return NULL;

	return brick->burst_get(brick, WEST_SIDE, pkts_mask);
}

struct rte_mbuf **brick_east_burst_get(struct brick *brick,
				       uint64_t *pkts_mask)
{
	if (!brick)
		return NULL;

	if (!brick->burst_get)
		return NULL;

	return brick->burst_get(brick, EAST_SIDE, pkts_mask);
}
