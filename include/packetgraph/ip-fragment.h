/* Copyright 2016 Outscale SAS
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

#ifndef _PG_IP_FRAGMENT_H
#define _PG_IP_FRAGMENT_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Create a new ip fragment brick
 *
 * @param   name name of the brick
 * @param   output side where packets can be fragmented,
 *          the oposite side is where packets are reasemble
 * @param   mtu_size allowed MTU size, must be a multiple of 8
 * @param   errp is set in case of an error
 * @return  a pointer to a brick, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_ip_fragment_new(const char *name,
				    enum pg_side output,
				    uint32_t mtu_size,
				    struct pg_error **errp);

#endif  /* _PG_IP_FRAGMENT_H */
