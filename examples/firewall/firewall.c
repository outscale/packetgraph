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

#include <packetgraph/packetgraph.h>

int main(int argc, char **argv)
{
	/**
	 * Simple example of firewall between two NICs.
	 * Print brick simply show packets.
	 *
	 * [nic-west]--[print-west]--[firewall]--[print-east]--[nic_east]
	 */

	struct pg_error *error = NULL;
	struct pg_brick *nic_west, *nic_east;
	struct pg_brick *print_west, *print_east;
	struct pg_brick *fw;
	struct pg_graph *graph;

	/* init packetgraph and nics */
	pg_start(argc, argv, &error);
	pg_nic_start();
	if (pg_nic_port_count() < 2) {
		printf("You need two DPDK ports to run this example\n");
		return 1;
	}

	/* create bricks */
	nic_west = pg_nic_new_by_id("nic-west", 0, &error);
	fw = pg_firewall_new("fw", PG_NO_CONN_WORKER, &error);
	print_west = pg_print_new("print-west", 0,
				  PG_PRINT_FLAG_MAX, 0, &error);
	print_east = pg_print_new("print-east", 0,
				  PG_PRINT_FLAG_MAX, 0, &error);
	nic_east = pg_nic_new_by_id("nic_east", 1, &error);

	/* link bricks */
	pg_brick_chained_links(&error, nic_west, print_west,
			       fw, print_east, nic_east);

	/* add some rules to firewall */
	pg_firewall_rule_add(fw, "tcp portrange 1-1024", PG_MAX_SIDE, 1, &error);
	pg_firewall_rule_add(fw, "icmp", PG_MAX_SIDE, 1, &error);
	pg_firewall_reload(fw, &error);

	/* create a graph with all bricks inside */
	graph = pg_graph_new("graph", fw, &error);

	printf("let's pool 1000*1000 times ...\n");
	for (int i = 0; i < 1000000; i++) {
		if (pg_graph_poll(graph, &error) < 0) {
			pg_error_print(error);
			pg_error_free(error);
		}
	}

	pg_graph_destroy(graph);
	pg_stop();
	return 0;
}
