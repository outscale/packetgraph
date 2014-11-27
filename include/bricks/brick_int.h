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

#include <ccan/autodata/autodata.h>
#include <ccan/build_assert/build_assert.h>

#include <rte_config.h>
#include <rte_mbuf.h>

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

enum side {
	WEST_SIDE = 0,
	EAST_SIDE = 1,
	MAX_SIDE  = 2,
};

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
	void (*burst)(struct brick *brick, enum side side,
		      struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask);
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
	int (*init)(struct brick *brick);	/* constructor
						 * return 1 on success,
						 * and 0 on error
						 */
	void (*destroy)(struct brick *brick);	/* destructor */

	int (*west_link)(struct brick *target,	/* link to west side */
			 struct brick *brick);
	int (*east_link)(struct brick *target,	/* link to east side */
			  struct brick *brick);
	void (*unlink)(struct brick *brick);	/* unlink */
};


/* brick_ops registration glue */
AUTODATA_TYPE(brick, struct brick_ops);

/**
 * Macro to register a brick operations structure
 *
 * @state_struct: the name of the structure containing the brick struct
 *		  This structure contains the brick private data and must start
 *		  with a struct brick brick;
 * @opt_struct:	  a pointer to a static brick_ops structure
 */

#define brick_register(state_struct, opt_struct) \
	char assert_failed_struct_brick_must_begin_state_struct \
		[(offsetof(state_struct, brick) == 0) ? 1 : -1]; \
	AUTODATA(brick, (opt_struct))

#endif
