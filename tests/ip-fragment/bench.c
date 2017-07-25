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
#include <packetgraph/ip-fragment.h>

#include "utils/bench.h"
#include "utils/bitmask.h"
#include "utils/network.h"
#include "packets.h"
#include "collect.h"
#include "utils/mempool.h"

static void test_benchmark_ip_fragment(int mtu, int max_burst_cnt,
				       const char *title,
				       int argc, char **argv)
{
	struct pg_error *error = NULL;
	struct pg_brick *ip_fragment;
	struct pg_bench bench;
	struct ether_addr mac = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x11} };
	uint32_t len;
	struct pg_bench_stats stats;
	int pkt_len = 1500;

	g_assert(!pg_bench_init(&bench, title, argc, argv, &error));
	ip_fragment = pg_ip_fragment_new("ip_fragment",
					 PG_EAST_SIDE, mtu, &error);
	g_assert(!error);

	bench.input_brick = ip_fragment;
	bench.input_side = PG_WEST_SIDE;
	bench.output_brick = ip_fragment;
	bench.output_side = PG_EAST_SIDE;
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
	bench.brick_full_burst = 1;
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
	pg_bench_print(&stats);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(ip_fragment);
	g_free(bench.pkts);
}

static void test_benchmark_ip_defragment(int mtu, const char *title,
					 int argc, char **argv)
{
	struct pg_error *error = NULL;
	struct pg_brick *ip_fragment;
	struct pg_brick *collect;
	struct pg_bench bench;
	struct ether_addr mac = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x11} };
	uint32_t len;
	struct pg_bench_stats stats;
	int pkt_len = 1500;
	struct rte_mbuf **tmp_pkts;
	uint64_t tmp_mask = 1;

	g_assert(!pg_bench_init(&bench, title, argc, argv, &error));
	ip_fragment = pg_ip_fragment_new("ip_fragment",
					 PG_WEST_SIDE, mtu, &error);
	g_assert(!error);
	collect = pg_collect_new("col_west", &error);
	g_assert(!error);

	pg_brick_link(collect, ip_fragment, &error);
	g_assert(!error);

	tmp_pkts = pg_packets_create(tmp_mask);
	tmp_pkts = pg_packets_append_ether(
		tmp_pkts,
		tmp_mask,
		&mac, &mac,
		ETHER_TYPE_IPv4);

	len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + pkt_len;
	pg_packets_append_ipv4(
		tmp_pkts,
		tmp_mask,
		0x000000EE, 0x000000CC, len, 17);
	tmp_pkts = pg_packets_append_blank(tmp_pkts, tmp_mask,
					   pkt_len);
	PG_FOREACH_BIT(tmp_mask, i) {
		struct eth_ipv4_hdr *pkt_buf =
			rte_pktmbuf_mtod(tmp_pkts[i], struct eth_ipv4_hdr *);
		pkt_buf->ip.fragment_offset = 0;
	}

	pg_brick_burst_to_west(ip_fragment, 0, tmp_pkts, tmp_mask, &error);
	bench.pkts = pg_brick_east_burst_get(collect, &bench.pkts_mask, &error);
	pg_packets_incref(bench.pkts, bench.pkts_mask ^ 1);

	bench.input_brick = ip_fragment;
	bench.input_side = PG_WEST_SIDE;
	bench.output_brick = ip_fragment;
	bench.output_side = PG_EAST_SIDE;
	bench.output_poll = false;
	bench.max_burst_cnt = 100000000;
	bench.count_brick = NULL;
	bench.pkts_nb = pg_mask_count(bench.pkts_mask);
	bench.brick_full_burst = 1;
	g_assert(pg_bench_run(&bench, &stats, &error) == 0);
	pg_bench_print(&stats);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(ip_fragment);
	pg_brick_destroy(collect);
	g_free(tmp_pkts);
}

static void benchmark_ip_frag_and_defragment(int mtu, const char *title,
					     int argc, char **argv)
{
	struct pg_error *error = NULL;
	struct pg_brick *ip_fragment;
	struct pg_brick *ip_reasemble;
	struct pg_bench bench;
	struct ether_addr mac = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x11} };
	uint32_t len;
	struct pg_bench_stats stats;
	int pkt_len = 1500;
	struct rte_mbuf **tmp_pkts;
	uint64_t tmp_mask = pg_mask_firsts(64);

	g_assert(!pg_bench_init(&bench, title, argc, argv, &error));
	ip_fragment = pg_ip_fragment_new("ip_fragment",
					 PG_EAST_SIDE, mtu, &error);

	ip_reasemble = pg_ip_fragment_new("ip_reasemble",
					  PG_WEST_SIDE, mtu, &error);

	pg_brick_link(ip_fragment, ip_reasemble,  &error);
	g_assert(!error);

	tmp_pkts = pg_packets_create(tmp_mask);
	tmp_pkts = pg_packets_append_ether(
		tmp_pkts,
		tmp_mask,
		&mac, &mac,
		ETHER_TYPE_IPv4);

	len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + pkt_len;
	pg_packets_append_ipv4(
		tmp_pkts,
		tmp_mask,
		0x000000EE, 0x000000CC, len, 17);
	tmp_pkts = pg_packets_append_blank(tmp_pkts, tmp_mask,
					   pkt_len);
	PG_FOREACH_BIT(tmp_mask, i) {
		struct eth_ipv4_hdr *pkt_buf =
			rte_pktmbuf_mtod(tmp_pkts[i], struct eth_ipv4_hdr *);
		pkt_buf->ip.fragment_offset = 0;
	}

	bench.pkts = tmp_pkts;
	bench.pkts_mask = tmp_mask;
	bench.input_brick = ip_fragment;
	bench.input_side = PG_WEST_SIDE;
	bench.output_brick = ip_reasemble;
	bench.output_side = PG_EAST_SIDE;
	bench.output_poll = false;
	bench.max_burst_cnt = 100000;
	bench.count_brick = NULL;
	bench.pkts_nb = pg_mask_count(bench.pkts_mask);
	bench.brick_full_burst = 1;
	pg_bench_run(&bench, &stats, &error);
	pg_error_print(error);
	pg_bench_print(&stats);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(ip_fragment);
	pg_brick_destroy(ip_reasemble);
	g_free(tmp_pkts);
}

int main(int argc, char **argv)
{
	int r;

	g_test_init(&argc, &argv, NULL);
	g_assert(pg_start(argc, argv) >= 0);
	test_benchmark_ip_fragment(1000, 1000000,
				   "fragment packets, mtu: 1000",
				   argc, argv);
	test_benchmark_ip_fragment(96, 100000,
				   "fragment packets, mtu: 96",
				   argc, argv);
	test_benchmark_ip_fragment(3000, 10000000,
				   "fragment packets, mtu: 3000",
				   argc, argv);
	test_benchmark_ip_defragment(3000,
				     "reassemble packets, mtu: 3000",
				     argc, argv);
	benchmark_ip_frag_and_defragment(1000,
				 "fragment and reassemble packets, mtu: 1000",
				 argc, argv);
	r = g_test_run();
	pg_stop();
	return r;
}
