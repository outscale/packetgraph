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

#ifndef _PG_CORE_GRAPH_H
#define _PG_CORE_GRAPH_H

#include <stdio.h>
#include <packetgraph/errors.h>

struct pg_brick;

/**
 * Write a dot (graphviz) graph to a file descriptor.
 *
 * @param	brick any brick pointer from which to start analyse the graph
 * @param	fd file descriptor where to write the graph description
 * @param	errp is set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_graph_dot(struct pg_brick *brick, FILE *fd, struct pg_error **errp);

#endif /* _PG_CORE_GRAPH_H */
