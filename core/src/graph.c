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
#include <packetgraph/graph.h>

int pg_graph_dot(struct pg_brick *brick, FILE *fd, struct pg_error **errp)
{
	GList *todo = NULL;
	GList *done = NULL;
	int i, j;

	fprintf(fd, "strict graph G {\n");
	fprintf(fd, "  rankdir=RL\n");
	fprintf(fd, "  nodesep=1\n");
	fprintf(fd, "  node [shape=record];\n");
	todo = g_list_append(todo, brick);

	while (todo != NULL) {
		/* Take the first brick to analyse. */
		struct pg_brick *b = todo->data;

		todo = g_list_remove(todo, b);

		/* declare node */
		fprintf(fd,
		"  \"%s:%s\" [ label=\"{ <west> | %s&#92;n%s |<east> }\"];\n",
			b->ops->name,
			b->name,
			b->ops->name,
			b->name);

		/* populate all connected bricks */
		for (i = 0; i < 2; i++)	{
			for (j = 0; j < b->sides[i].max; j++) {
				struct pg_brick *n = b->sides[i].edges[j].link;

				if (!n)
					continue;
				/* A new brick appears */
				if (!g_list_find(todo, n) &&
				    !g_list_find(done, n)) {
					todo = g_list_append(todo, n);
				}
				fprintf(fd, "  \"%s:%s\":%s -- \"%s:%s\":%s\n",
					b->ops->name,
					b->name,
					(i == WEST_SIDE ? "west" : "east"),
					n->ops->name,
					n->name,
					(i == WEST_SIDE ? "east" : "west"));
			}
		}
		/* mark this brick as done */
		done = g_list_append(done, b);
	}
	fprintf(fd, "}\n");
	return 1;
}

