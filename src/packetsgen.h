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

#ifndef _PG_CORE_PACKETSGEN_H
#define _PG_CORE_PACKETSGEN_H

#include <rte_config.h>
#include <rte_mbuf.h>
#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Create a new packetsgen brick, a packet generator brick.
 * This is not a common brick as it is mainly used for testing and debuging.
 * Note that all packets provided here are cloned before being burst.
 * Your will need to free provided packets once the brick is destroyed.
 * Bursted packets contains a growing counter in it's user data (used for
 * testing purposes).
 *
 * @param	name name of the brick
 * @param	west_max maximum of links you can connect on the west side
 * @param	east_max maximum of links you can connect on the east side
 * @param	output direction where packets are generated
 * @param	packets a pointer to a packet array
 * @param	packets_nb size of packet array
 * @param	errp is set in case of an error
 * @return	a pointer to a brick structure, on success, NULL on error
 */
struct pg_brick *pg_packetsgen_new(const char *name,
				   uint32_t west_max,
				   uint32_t east_max,
				   enum pg_side output,
				   struct rte_mbuf **packets,
				   uint16_t packets_nb,
				   struct pg_error **errp);

#endif  /* _PG_CORE_PACKETSGEN_H */
