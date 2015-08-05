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

#include <packetgraph/brick.h>

/**
 * Write a dot (graphviz) graph to a file descriptor.
 *
 * @param	brick any brick pointer from which to start analyse the graph
 * @param	fd file descriptor where to write the graph description
 * @param	errp is set in case of an error
 * @return	1 on success, 0 on error
 */
int graph_dot(struct brick *brick, FILE *fd, struct switch_error **errp);

