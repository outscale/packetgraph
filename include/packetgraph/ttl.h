/* Copyright 2015 Outscale SAS
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

#ifndef _PG_TTL_H
#define _PG_TTL_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>
//#include <rte_mbuf.h>

/**
 * Create a new ttl brick
 *
 * @param   name brick's name
 * @param   errp is set in case of an error
 * @return  a pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED

struct pg_brick *pg_ttl_new(const char *name, struct pg_error **errp);
void pg_ttl_handle(struct pg_brick *brick, struct rte_mbuf *pkt,
		struct pg_error **errp);

#endif  /* _PG_TTL_H */
