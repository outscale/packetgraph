/* Copyright 2015 Outscale SAS
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

#ifndef _PG_CORE_PACKETGRAPH_H
#define _PG_CORE_PACKETGRAPH_H

#include <stdarg.h>
#include <packetgraph/common.h>
#include <packetgraph/errors.h>
#include <packetgraph/graph.h>

struct pg_brick;

/* testing */
int64_t pg_brick_refcount(struct pg_brick *brick);

/**
 * Initialize packetgraph.
 * This function should be called before any other packetgraph function.
 *
 * @param	size of argv
 * @param	argv all arguments passwed to packetgraph, it may contain
 *		DPDK arugments.
 * @param	errp is set in case of an error
 * @return	return where the program's parameters count starts
 */
int pg_start(int argc, char **argv, struct pg_error **errp);

/**
 * Uninitialize packetgraph.
 * This function should be called when the application exits.
 */
void pg_stop(void);

/**
 * Revert side: WEST_SIDE become EAST_SIDE and vice versa.
 */
static inline enum pg_side pg_flip_side(enum pg_side side)
{
	return (enum pg_side)(side ^ 1);
}

/**
 * Link two bricks with each others.
 *
 * @param	west the brick wich will be linked to it's east side to the
 *		other brick.
 * @param	east the brick wich will be linked to it's west side to the
 *		other brick.
 * @param	errp is set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_brick_link(struct pg_brick *west, struct pg_brick *east,
		  struct pg_error **errp);

/**
 * Link a serie of bricks together.
 * If you call brick_chained_links(errp, a, b, c)
 * you will get this graph:
 * [a] -- [b] -- [c]
 *
 * @param	errp is set in case of an error
 * @param	west the brick on the wester side
 * @param	args the other bricks
 * @return	1 on success, 0 on error
 */
#define pg_brick_chained_links(errp, west, args...)	\
	(pg_brick_chained_links_int(errp, west, args, NULL))

/**
 * Internal function for brick_chained_links
 *
 * Same as the macro brick_chained_links, but don't add
 * NULL automatically in the last parameter.
 * Therefore this function should not be call,
 * Call brick_chained_links instead.
 */
int pg_brick_chained_links_int(struct pg_error **errp,
			       struct pg_brick *west, ...);

/**
 * Unlink a brick from all it's connected neighbours.
 *
 * @param	brick brick which will be unlinked
 * @param	errp is set in case of an error
 */
void pg_brick_unlink(struct pg_brick *brick, struct pg_error **errp);

/**
 * Poll packets from a brick a let it flow through the graph.
 * All polled packets will be free once the poll is done.
 *
 * @param	brick brick to poll
 * @param	count number of polled packets
 * @param	errp is set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_brick_poll(struct pg_brick *brick, uint16_t *count,
		  struct pg_error **errp);

/**
 * Number packets received by a specific side.
 *
 * @param	brick brick pointer
 * @param	side wi
 * @param	errp is set in case of an error
 * @return	number of packets the brick got on the specified side.
 */
uint64_t pg_brick_pkts_count_get(struct pg_brick *brick, enum pg_side side);

/**
 * Delete a brick.
 *
 * @param	brick brick pointer
 */
void pg_brick_destroy(struct pg_brick *brick);

const char *pg_brick_name(struct pg_brick *brick);
const char *pg_brick_type(struct pg_brick *brick);

#endif /* _PG_CORE_PACKETGRAPH_H */
