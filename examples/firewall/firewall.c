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

#include <glib.h>
#include <sys/time.h>
#include <time.h>
#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_eth_ring.h>
#include <unistd.h>

#include <packetgraph/packetgraph.h>
#include <packetgraph/nic.h>
#include <packetgraph/firewall.h>

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)

#define LOOPS 100000

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	struct pg_brick *fw;
	struct pg_brick *nic_west, *nic_east;
	uint16_t nb_send_pkts;
	uint64_t total;
	struct timeval start, end;
	int i;

	pg_start(argc, argv, &error);
	CHECK_ERROR(error);
	g_assert(rte_eth_dev_count() >= 2);

	nic_west = pg_nic_new_by_id("port 0", 1, 1, WEST_SIDE, 0, &error);
	CHECK_ERROR(error);
	fw = pg_firewall_new("fw", 1, 1, PG_NO_CONN_WORKER, &error);
	CHECK_ERROR(error);
	nic_east = pg_nic_new_by_id("port 1", 1, 1, EAST_SIDE, 1, &error);
	CHECK_ERROR(error);

	pg_brick_link(nic_west, fw, &error);
	CHECK_ERROR(error);
	pg_brick_link(fw, nic_east, &error);
	CHECK_ERROR(error);

	g_assert(!pg_firewall_rule_add(fw, "tcp portrange 50-60", MAX_SIDE, 1,
				       &error));
	CHECK_ERROR(error);
	g_assert(!pg_firewall_rule_add(fw, "icmp", MAX_SIDE, 1, &error));
	CHECK_ERROR(error);
	g_assert(!pg_firewall_reload(fw, &error));
	CHECK_ERROR(error);

	for (;;) {
		gettimeofday(&start, 0);
		total = 0;
		for (i = 0; i < LOOPS; i++) {
			g_assert(pg_brick_poll(nic_west,
					       &nb_send_pkts,
					       &error));
			usleep(1);
			total += nb_send_pkts;
			g_assert(pg_brick_poll(nic_east,
					       &nb_send_pkts,
					       &error));
			total += nb_send_pkts;
			usleep(1);
		}
		gettimeofday(&end, 0);
		usleep(100);
		printf("time in us: for %i loops: %lu\ntotal %"PRIu64"\n",
		       LOOPS,
		       (end.tv_sec * 1000000 + end.tv_usec) -
		       (start.tv_sec * 1000000 + start.tv_usec), total);
		pg_firewall_gc(fw);
	}

	pg_stop();
	return 0;
}

#undef LOOPS
