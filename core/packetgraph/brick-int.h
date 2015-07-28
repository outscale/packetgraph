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

#ifndef _BRICKS_BRICK_INT_H_
#define _BRICKS_BRICK_INT_H_

#include <glib.h>
#include <stdint.h>
#include <rte_config.h>
#include <rte_mbuf.h>
#include <rte_atomic.h>
#include <packetgraph/common.h>
#include <packetgraph/utils/config.h>
#include <packetgraph/utils/errors.h>
#include <packetgraph/ccan/build_assert/build_assert.h>

struct brick;

/* testing */
int64_t brick_refcount(struct brick *brick);


/**
 * Each step in the pipeline from the physical network card to the guest vm
 * uplink and backward is called a brick. A brick can be a switch, a hub, a
 * firewall or whatever network component is required. Packets can flow in two
 * directions west or east.
 */

#define BRICK_NAME_LENGTH	32

/**
 * The structure containing a brick private data must start with a
 * struct brick brick; that will be used to manipulate the brick brick.
 *
 * The following utility function is used to get a pointer to the private
 * state structure given a struct brick pointer.
 */
#define brick_get_state(ptr, type) ({			\
	BUILD_ASSERT(offsetof(type, brick) == 0);	\
	(type *) ptr; })

struct brick_ops;

/* The end of an edge linking two struct brick */
struct brick_edge {
	struct brick *link;		/* paired struct brick */
	uint32_t pair_index;		/* index of the paired brick_edge */
	uint32_t padding;		/* for 64 bits alignment */
};

struct brick_side {
	uint16_t max;			/* maximum number of edges */
	uint16_t nb;			/* number of edges */
	struct brick_edge *edges;	/* edges */
	rte_atomic64_t packet_count;	/* incoming pkts count */
};

/**
 * Brick State
 *
 * This structure contains the configuration and edges of a brick and
 * its private data.
 */
struct brick {
	/**
	 * The four following callback are embedded in the brick brick in order
	 * to minimize the numbers of indirections because they are fast paths.
	 */

	/* Accept a packet burst */
	int (*burst)(struct brick *brick, enum side from, uint16_t edge_index,
		     struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		     struct switch_error **errp);
	/* polling */
	int (*poll)(struct brick *brick,
		    uint16_t *count, struct switch_error **errp);

	/**
	 * Return a packet burst. This field is used bricks designed to
	 * collect packets for testing purpose. In regular bricks it will be
	 * set to NULL.
	 */
	struct rte_mbuf **(*burst_get)(struct brick *brick, enum side side,
				       uint64_t *pkts_mask);

	struct brick_side sides[2];	/* east and west sides */

	struct brick_ops *ops;		/* management ops */
	int64_t refcount;		/* reference count */
	char *name;			/* unique name */
};

/**
 * Brick Operations
 *
 * This structure contains pointers to the management operations that can be
 * done on a brick. Its the slow path.
 *
 */
struct brick_ops {
	char name[BRICK_NAME_LENGTH];		/* Flawfinder: ignore */
						/* note: brick_ops structures
						 * will always be declared
						 * statically so will be name.
						 */

	ssize_t state_size;			/* private state size */

	/* life cycle */
	int (*init)(struct brick *brick,		/* constructor */
		    struct brick_config *config,	/* ret 1 on success */
		    struct switch_error **errp);	/* and 0 on error */
	void (*destroy)(struct brick *brick,		/* destructor */
			struct switch_error **errp);

	void (*unlink)(struct brick *brick, struct switch_error **errp);

	/* called to notify a brick that a neighbor broke a link with it */
	void (*unlink_notify)(struct brick *brick, enum side unlinker_side,
			      uint16_t edge_index, struct switch_error **errp);

	int (*reset)(struct brick *brick, struct switch_error **errp);

	/* this return a copy of the brick handle:
	 * the socket path for vhost-user.
	 */
	char *(*handle_dup)(struct brick *brick, struct switch_error **errp);
};

extern GList *packetgraph_all_bricks;
/**
 * Macro to register a brick operations structure
 * @brickname:	  name of the brick
 * @ops:	  a pointer to a static brick_ops structure
 */
#define brick_register(brickname, ops) \
	static void brick_##brickname##_register(void) \
		__attribute__((constructor(101))); \
	static void brick_##brickname##_register(void) \
	{ \
		packetgraph_all_bricks = g_list_append(packetgraph_all_bricks, \
						       (ops)); \
	}

#endif

