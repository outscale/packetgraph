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

#include <rte_config.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <glib.h>
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include <packetgraph/pmtud.h>
#include "utils/bench.h"
#include "utils/bitmask.h"
#include "packets.h"

static void test_benchmark_pmtud(int argc, char **argv)
{
	struct pg_error *error = NULL;
	struct pg_brick *pmtud;
	struct pg_bench bench;
	struct ether_addr mac = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x11} };
	uint32_t len;
	struct pg_bench_stats stats;

	g_assert(!pg_bench_init(&bench, "pmtud", argc, argv, &error));
	pmtud = pg_pmtud_new("pmtud", PG_WEST_SIDE, 1500, &error);
	g_assert(!error);

	bench.input_brick = pmtud;
	bench.input_side = PG_WEST_SIDE;
	bench.output_brick = pmtud;
	bench.output_side = PG_EAST_SIDE;
	bench.output_poll = false;
	bench.max_burst_cnt = 10000000;
	bench.count_brick = NULL;
	bench.pkts_nb = 64;
	bench.pkts_mask = pg_mask_firsts(64);
	bench.pkts = pg_packets_create(bench.pkts_mask);
	bench.pkts = pg_packets_append_ether(
		bench.pkts,
		bench.pkts_mask,
		&mac, &mac,
		ETHER_TYPE_IPv4);
	bench.brick_full_burst = 1;
	len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + 1400;
	pg_packets_append_ipv4(
		bench.pkts,
		bench.pkts_mask,
		0x000000EE, 0x000000CC, len, 17);
	bench.pkts = pg_packets_append_udp(
		bench.pkts,
		bench.pkts_mask,
		1000, 2000, 1400);
	bench.pkts = pg_packets_append_blank(bench.pkts, bench.pkts_mask, 1400);

	g_assert(pg_bench_run(&bench, &stats, &error) == 0);
	pg_bench_print(&stats);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(pmtud);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_assert(pg_start(argc, argv) >= 0);
	test_benchmark_pmtud(argc, argv);
	int r = g_test_run();

	pg_stop();
	return r;
}
