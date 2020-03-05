/* Copyright 2020 Outscale SAS
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

#include <glib.h>

#include <rte_config.h>
#include <rte_ether.h>

#include <packetgraph/packetgraph.h>
#include <packetgraph/udp-filter.h>

#include "utils/tests.h"
#include "collect.h"
#include "packets.h"
#include "utils/common.h"
#include "brick-int.h"

static void test_udp_filter(void)
{
	struct pg_error *error = NULL;
	struct rte_mbuf **pkts = pg_packets_create(-1LL);
	struct rte_mbuf **result_pkts;
	uint64_t pkts_mask;

	pg_autobrick struct pg_brick *collect_east0 =
		pg_collect_new("collect-east0", &error);
	pg_autobrick struct pg_brick *collect_east1 =
		pg_collect_new("collect-east1", &error);
	pg_autobrick struct pg_brick *sw =
		pg_udp_filter_new(
			"sw_udp",
			(struct pg_udp_filter_info [])
			{{68, 67, PG_USP_FILTER_SRC_PORT}},
			1, PG_EAST_SIDE, &error);

	pg_packets_append_ether(pkts, -1LL,
				&(struct ether_addr){{0, 1}},
				&(struct ether_addr){{0xff}},
				ETHER_TYPE_IPv4
		);
	pg_packets_append_ipv4(pkts, -1LL, 0, -1, 10, 17);
	pg_packets_append_udp(pkts, 0xffffffffLU, 68, 67, 1000);
	pg_packets_append_udp(pkts, ~0xffffffffLU, 60, 61, 1000);

	pg_brick_link(sw, collect_east0, &error);
	pg_brick_link(sw, collect_east1, &error);

	pg_brick_burst_to_east(sw, 0, pkts, -1LL, &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	result_pkts = pg_brick_west_burst_get(collect_east0, &pkts_mask,
					      &error);
	g_assert(pkts_mask == ~0xffffffffLU);
	g_assert(result_pkts);

	result_pkts = pg_brick_west_burst_get(collect_east1, &pkts_mask,
					      &error);
	g_assert(pkts_mask == 0xffffffffLLU);
	g_assert(result_pkts);

	pg_packets_free(pkts, -1LL);
}

int main(int argc, char **argv)
{
	int r;

	g_test_init(&argc, &argv, NULL);
	g_assert(pg_start(argc, argv) >= 0);

	pg_test_add_func("/udp-filtering",
			 test_udp_filter);

	r = g_test_run();

	pg_stop();
	return r;
}
