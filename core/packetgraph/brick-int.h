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

#ifndef _PG_CORE_BRICK_INT_H
#define _PG_CORE_BRICK_INT_H

#include <glib.h>
#include <stdint.h>
#include <rte_config.h>
#include <rte_mbuf.h>
#include <rte_atomic.h>
#include <packetgraph/common.h>
#include <packetgraph/utils/config.h>
#include <packetgraph/utils/errors.h>
#include <packetgraph/ccan/build_assert/build_assert.h>

struct pg_brick;


/**
 * Each step in the pipeline from the physical network card to the guest vm
 * uplink and backward is called a brick. A brick can be a switch, a hub, a
 * firewall or whatever network component is required. Packets can flow in two
 * directions west or east.
 */

#define BRICK_NAME_LENGTH	32

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

struct pg_brick_ops;

/* The end of an edge linking two struct pg_brick */
struct pg_brick_edge {
	struct pg_brick *link;		/* paired struct pg_brick */
	uint32_t pair_index;		/* index of the paired brick_edge */
	uint32_t padding;		/* for 64 bits alignment */
};

struct pg_brick_side {
	rte_atomic64_t packet_count;	/* incoming pkts count */
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
		struct pg_brick_side sides[MAX_SIDE];
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
	char name[BRICK_NAME_LENGTH];		/* Flawfinder: ignore */
						/* note: brick_ops structures
						 * will always be declared
						 * statically so will be name.
						 */

	ssize_t state_size;			/* private state size */

	/* life cycle */
	int (*init)(struct pg_brick *brick,		/* constructor */
		    struct pg_brick_config *config,	/* ret 1 on success */
		    struct pg_error **errp);		/* and 0 on error */

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

#endif /* _PG_CORE_BRICK_INT_H */

