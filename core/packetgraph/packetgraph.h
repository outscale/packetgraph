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

#ifndef _PACKETGRAPH_H_
#define _PACKETGRAPH_H_

#include <stdarg.h>
#include <packetgraph/utils/errors.h>
#include <packetgraph/utils/config.h>
#include <packetgraph/brick-int.h>

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
int packetgraph_start(int argc, char **argv, struct switch_error **errp);

/**
 * Uninitialize packetgraph.
 * This function should be called when the application exits.
 */
void packetgraph_stop(void);

/**
 * Revert side: WEST_SIDE become EAST_SIDE and vice versa.
 */
enum side flip_side(enum side side);

/**
 * Link two bricks with each others.
 *
 * @param	west the brick wich will be linked to it's east side to the
 *		other brick.
 * @param	east the brick wich will be linked to it's west side to the
 *		other brick.
 * @param	errp is set in case of an error
 * @return	1 on success, 0 on error
 */
int brick_link(struct brick *west, struct brick *east,
	       struct switch_error **errp);

#define brick_chained_links(errp, west, args...)	\
	(brick_chained_links_int(errp, west, args, NULL))

/**
 * Link a serie of bricks together.
 *
 * @param	errp is set in case of an error
 * @param	west the brick wich will be linked to it's east side to the
 *		other brick.
 * @return	1 on success, 0 on error
 */
int brick_chained_links_int(struct switch_error **errp,
		struct brick *west, ...);

/**
 * Unlink a brick from all it's connected neighbours.
 *
 * @param	brick brick which will be unlinked
 * @param	errp is set in case of an error
 */
void brick_unlink(struct brick *brick, struct switch_error **errp);

/**
 * Poll packets from a brick a let it flow through the graph.
 * All polled packets will be free once the poll is done.
 *
 * @param	brick brick to poll
 * @param	count number of polled packets
 * @param	errp is set in case of an error
 * @return	1 on success, 0 on error
 */
int brick_poll(struct brick *brick, uint16_t *count,
	       struct switch_error **errp);

/**
 * Number packets received by a specific side.
 *
 * @param	brick brick pointer
 * @param	side wi
 * @param	errp is set in case of an error
 * @return	number of packets the brick got on the specified side.
 */
uint64_t brick_pkts_count_get(struct brick *brick, enum side side);

/**
 * Delete a brick.
 *
 * @param	brick brick pointer
 */
void brick_destroy(struct brick *brick);

#endif
