/* Copyright 2016 Outscale SAS
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

#include <rte_config.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <glib.h>
#include <packetgraph/packetgraph.h>
#include <packetgraph/ip-fragment.h>
#include "utils/bench.h"
#include "utils/bitmask.h"
#include "packets.h"

struct eth_ipv4_hdr {
	struct ether_hdr eth;
	struct ipv4_hdr ip;
} __attribute__((__packed__));

static void test_benchmark_ip_fragment(int mtu, int max_burst_cnt)
{
	struct pg_error *error = NULL;
	struct pg_brick *ip_fragment;
	struct pg_bench bench;
	struct ether_addr mac = {{0x52,0x54,0x00,0x12,0x34,0x11}};
	uint32_t len;
	struct pg_bench_stats stats;
	int pkt_len = 1500;

	pg_bench_init(&bench);
	ip_fragment = pg_ip_fragment_new("ip_fragment",
					 EAST_SIDE, mtu, &error);
	g_assert(!error);

	bench.input_brick = ip_fragment;
	bench.input_side = WEST_SIDE;
	bench.output_brick = ip_fragment;
	bench.output_side = EAST_SIDE;
	bench.output_poll = false;
	bench.max_burst_cnt = max_burst_cnt;
	bench.count_brick = NULL;
	bench.pkts_nb = 64;
	bench.pkts_mask = pg_mask_firsts(64);
	bench.pkts = pg_packets_create(bench.pkts_mask);
	bench.pkts = pg_packets_append_ether(
		bench.pkts,
		bench.pkts_mask,
		&mac, &mac,
		ETHER_TYPE_IPv4);
	len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + pkt_len;
	pg_packets_append_ipv4(
		bench.pkts,
		bench.pkts_mask,
		0x000000EE, 0x000000CC, len, 17);
	bench.pkts = pg_packets_append_blank(bench.pkts, bench.pkts_mask,
					     pkt_len);
	PG_FOREACH_BIT(bench.pkts_mask, i) {
		struct eth_ipv4_hdr *pkt_buf =
			rte_pktmbuf_mtod(bench.pkts[i], struct eth_ipv4_hdr *);
		pkt_buf->ip.fragment_offset = 0;
	}

	g_assert(pg_bench_run(&bench, &stats, &error) == 0);
	/* We know that this brick burst all packets. */
	stats.pkts_burst = stats.pkts_sent;
	g_assert(pg_bench_print(&stats, NULL) == 0);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(ip_fragment);
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int r;

	g_test_init(&argc, &argv, NULL);
	pg_start(argc, argv, &error);
	g_assert(!error);
	printf("fragment packets, mtu: 1000\n");
	test_benchmark_ip_fragment(1000, 1000000);
	printf("fragment packets, mtu: 96\n");
	test_benchmark_ip_fragment(96, 100000);
	printf("fragment packets, mtu: 3000\n");
	test_benchmark_ip_fragment(3000, 10000000);
	r = g_test_run();
	pg_stop();
	return r;
}
