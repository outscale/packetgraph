/* Copyright 2014 Nodalink EURL
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

#ifndef _PG_BRICK_INT_H
#define _PG_BRICK_INT_H

#include <glib.h>
#include <stdint.h>
#include <rte_config.h>
#include <rte_mbuf.h>
#include <rte_atomic.h>
#include <packetgraph/common.h>
#include <packetgraph/errors.h>
#include <packetgraph/brick.h>
#include "utils/config.h"
#include "utils/common.h"
#include "utils/ccan/build_assert/build_assert.h"

/**
 * Each step in the pipeline from the physical network card to the guest vm
 * uplink and backward is called a brick. A brick can be a switch, a hub, a
 * firewall or whatever network component is required. Packets can flow in two
 * directions west or east.
 */


/**
 * The structure containing a brick private data must start with a
 * struct pg_brick brick; that will be used to manipulate the brick brick.
 *
 * The following utility function is used to get a pointer to the private
 * state structure given a struct pg_brick pointer.
 */
#define pg_brick_get_state(ptr, type) ({		\
	BUILD_ASSERT(offsetof(type, brick) == 0);	\
	(type *) ptr; })

#ifdef PG_BRICK_NO_ATOMIC_COUNT
#define PG_PKTS_COUNT_GET(X) (X)
#define PG_PKTS_COUNT_TYPE uint64_t
#define PG_PKTS_COUNT_ADD(X, Y) (X += Y)
#define PG_PKTS_COUNT_SET(X, Y) (X = Y)
#else
#define PG_PKTS_COUNT_TYPE rte_atomic64_t
#define PG_PKTS_COUNT_GET(X) rte_atomic64_read(&X)
#define PG_PKTS_COUNT_ADD(X, Y) rte_atomic64_add(&X, Y)
#define PG_PKTS_COUNT_SET(X, Y) rte_atomic64_set(&X, Y)
#endif

struct pg_brick_ops;

/* The end of an edge linking two struct pg_brick */
struct pg_brick_edge {
	struct pg_brick *link;		/* paired struct pg_brick */
	uint32_t pair_index;		/* index of the paired brick_edge */
	uint32_t padding;		/* for 64 bits alignment */
};

struct pg_brick_side {
	PG_PKTS_COUNT_TYPE packet_count;	/* incoming pkts count */
	/* Optional callback to set to get the number of packets which has been
	 * bursted/enqueue. Default: NULL.
	 */
	void (*burst_count_cb)(void *private_data, uint16_t burst_count);
	/* Private data to provide when using callback. */
	void *burst_count_private_data;
	/* a padding which can contain any kind of side */
	uint16_t max;			/* maximum number of edges */
	uint16_t nb;			/* number of edges */

	/* Edges is use by multipoles bricks,
	 * and edge by dipole and monopole bricks
	 */
	union {
		struct pg_brick_edge *edges;	/* edges */
		struct pg_brick_edge edge;
	};
};


/**
 * Brick State
 *
 * This structure contains the configuration and edges of a brick and
 * its private data.
 */
struct pg_brick {
	/**
	 * The four following callback are embedded in the brick brick in order
	 * to minimize the numbers of indirections because they are fast paths.
	 */

	/* Accept a packet burst */
	int (*burst)(struct pg_brick *brick, enum pg_side from,
		     uint16_t edge_index, struct rte_mbuf **pkts,
		     uint64_t pkts_mask, struct pg_error **errp);
	/* polling */
	int (*poll)(struct pg_brick *brick,
		    uint16_t *count, struct pg_error **errp);

	struct pg_brick_ops *ops;	/* management ops */
	int64_t refcount;		/* reference count */
	char *name;			/* unique name */
	enum pg_brick_type type;

	/* Sides is use by multipoles and dipole bricks,
	 * and side by monopole bricks
	 */
	union {
		struct pg_brick_side sides[PG_MAX_SIDE];
		struct pg_brick_side side;
	};
};


/**
 * Brick Operations
 *
 * This structure contains pointers to the management operations that can be
 * done on a brick. Its the slow path.
 *
 */
struct pg_brick_ops {
	const char *name;			/* Flawfinder: ignore */
						/* note: brick_ops structures
						 * will always be declared
						 * statically so will be name.
						 */

	ssize_t state_size;			/* private state size */

	/* life cycle */
	int (*init)(struct pg_brick *brick,		/* constructor */
		    struct pg_brick_config *config,	/* ret 0 on success */
		    struct pg_error **errp);		/* and -1 on error */

	void (*destroy)(struct pg_brick *brick,		/* destructor */
			struct pg_error **errp);

	void (*unlink)(struct pg_brick *brick, struct pg_error **errp);

	/* called to notify a brick that a neighbor broke a link with it */
	void (*unlink_notify)(struct pg_brick *brick,
			      enum pg_side unlinker_side,
			      uint16_t edge_index, struct pg_error **errp);

	int (*reset)(struct pg_brick *brick, struct pg_error **errp);

	void (*link_notify)(struct pg_brick *brick, enum pg_side side,
			    int index);
	enum pg_side (*get_side)(struct pg_brick *brick);

	/**
	 * Return a packet burst. This field is used bricks designed to
	 * collect packets for testing purpose. In regular bricks it will be
	 * set to NULL.
	 */
	struct rte_mbuf **(*burst_get)(struct pg_brick *brick,
				       enum pg_side side,
				       uint64_t *pkts_mask);

	/* If set, the brick is able to return the number of transfered bytes
	 * This is useful for some monopole bricks.
	 * See pg_brick_rx_bytes and pg_brick_tx_bytes
	 */
	uint64_t (*rx_bytes)(struct pg_brick *brick);
	uint64_t (*tx_bytes)(struct pg_brick *brick);
};

extern GList *pg_all_bricks;
/**
 * Macro to register a brick operations structure
 * @brickname:	  name of the brick
 * @ops:	  a pointer to a static brick_ops structure
 */
#define pg_brick_register(brickname, ops) \
	static void pg_##brickname##_brick_register(void) \
		__attribute__((constructor(101))); \
	static void pg_##brickname##_brick_register(void) \
	{ \
		pg_all_bricks = g_list_append(pg_all_bricks, (ops)); \
	}

 /* lifecycle */
PG_WARN_UNUSED
struct pg_brick *pg_brick_new(const char *name,
			      struct pg_brick_config *config,
			      struct pg_error **errp);

void pg_brick_incref(struct pg_brick *brick);
struct pg_brick *pg_brick_decref(struct pg_brick *brick,
				 struct pg_error **errp);
int pg_brick_reset(struct pg_brick *brick, struct pg_error **errp);

/* testing */
uint32_t pg_brick_links_count_get(const struct pg_brick *brick,
				  const struct pg_brick *target,
				  struct pg_error **errp);
int64_t pg_brick_refcount(const struct pg_brick *brick);

/* generic functions used to factorize code */

/* these are generic brick_ops callbacks */
int pg_brick_generic_west_link(struct pg_brick *target,
			       struct pg_brick *brick,
			       struct pg_error **errp);
int pg_brick_generic_east_link(struct pg_brick *target,
			       struct pg_brick *brick,
			       struct pg_error **errp);
void pg_brick_generic_unlink(struct pg_brick *brick, struct pg_error **errp);

/* data flow */
int pg_brick_burst(struct pg_brick *brick, enum pg_side from,
		   uint16_t edge_index,
		   struct rte_mbuf **pkts, uint64_t pkts_mask,
		   struct pg_error **errp);

/* data flow commodity functions */
int pg_brick_burst_to_east(struct pg_brick *brick, uint16_t edge_index,
			struct rte_mbuf **pkts, uint64_t pkts_mask,
			struct pg_error **errp);

int pg_brick_burst_to_west(struct pg_brick *brick, uint16_t edge_index,
			   struct rte_mbuf **pkts, uint64_t pkts_mask,
			   struct pg_error **errp);

/* used for testing */
struct rte_mbuf **pg_brick_west_burst_get(struct pg_brick *brick,
					  uint64_t *pkts_mask,
					  struct pg_error **errp);

struct rte_mbuf **pg_brick_east_burst_get(struct pg_brick *brick,
					  uint64_t *pkts_mask,
					  struct pg_error **errp);

int pg_brick_side_forward(struct pg_brick_side *brick_side, enum pg_side from,
			  struct rte_mbuf **pkts, uint64_t pkts_mask,
			  struct pg_error **errp);

/**
 * Get the edge of a brick.
 * @brick:	the brick
 * @side:	the side on which the edge is store, useless if monopole brick
 * @edge:	the edge we want, useless if not a miltipole brick.
 */
struct pg_brick_edge *pg_brick_get_edge(struct pg_brick *brick,
					enum pg_side side,
					uint32_t edge);

/**
 * return the maximum number of brick a side can have
 */
uint32_t pg_side_get_max(const struct pg_brick *brick, enum pg_side side);


struct pg_brick_edge_iterator {
	struct pg_brick *brick;
	uint32_t edge;
	enum pg_side side;
	bool end;
};

static inline struct pg_brick_edge *
pg_brick_edge_iterator_get(struct pg_brick_edge_iterator *it)
{
	return pg_brick_get_edge(it->brick, it->side, it->edge);
}

static inline void
pg_brick_edge_iterator_next(struct pg_brick_edge_iterator *it)
{
	if (it->end)
		return;
	if (it->brick->type == PG_MONOPOLE) {
		it->end = true;
		return;
	}
	it->edge += 1;
	if (it->edge >= pg_side_get_max(it->brick, it->side)) {
		it->side += 1;
		it->edge = 0;
	}
	if (it->side == PG_MAX_SIDE) {
		it->end = true;
		return;
	}
	struct pg_brick_edge *e = pg_brick_edge_iterator_get(it);

	if (!e || !e->link) {
		pg_brick_edge_iterator_next(it);
		return;
	}
}

static inline void
pg_brick_edge_iterator_init(struct pg_brick_edge_iterator *it,
			    struct pg_brick *brick)
{
	it->brick = brick;
	it->end = false;
	it->edge = 0;
	it->side = 0;
	struct pg_brick_edge *e = pg_brick_edge_iterator_get(it);

	if (!e || !e->link)
		pg_brick_edge_iterator_next(it);
}

static inline bool
pg_brick_edge_iterator_is_end(struct pg_brick_edge_iterator *it)
{
	return it->end;
}

static inline struct pg_brick_side *
pg_brick_edge_iterator_get_side(struct pg_brick_edge_iterator *it)
{
	switch (it->brick->type) {
	case PG_MULTIPOLE:
	case PG_DIPOLE:
		return &it->brick->sides[it->side];
	case PG_MONOPOLE:
		return &it->brick->side;
	}
	g_assert(0);
}

#define PG_BRICK_FOREACH_EDGES(brick, it)		\
	struct pg_brick_edge_iterator it;		\
for (pg_brick_edge_iterator_init(&it, brick);		\
	!pg_brick_edge_iterator_is_end(&it);		\
	pg_brick_edge_iterator_next(&it))


#endif /* _PG_BRICK_INT_H */

