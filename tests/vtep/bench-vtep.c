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

#include <netinet/in.h>
#include <arpa/inet.h>
#include <rte_config.h>
#include <rte_common.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include <packetgraph/vtep.h>
#include <packetgraph/nop.h>
#include "packets.h"
#include "brick-int.h"
#include "utils/bench.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/ip.h"

void test_benchmark_vtep(int argc, char **argv);

/**
 * Composite structure of all the headers required to wrap a packet in VTEP
 */
struct headers4 {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	struct ipv4_hdr	 ipv4; /* define in rte_ip.h */
	struct udp_hdr	 udp; /* define in rte_udp.h */
	struct vxlan_hdr vxlan; /* define in rte_ether.h */
} __attribute__((__packed__));

struct headers6 {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	struct ipv6_hdr	 ipv6; /* define in rte_ip.h */
	struct udp_hdr	 udp; /* define in rte_udp.h */
	struct vxlan_hdr vxlan; /* define in rte_ether.h */
} __attribute__((__packed__));

struct igmp_hdr {
	uint8_t type;
	uint8_t maxRespTime;
	uint16_t checksum;
	uint32_t groupAddr;
} __attribute__((__packed__));

struct multicast_pkt {
	struct ether_hdr ethernet;
	struct ipv4_hdr ipv4;
	struct igmp_hdr igmp;
} __attribute__((__packed__));


int headers_length;

static void remove_vtep_hdr(struct pg_bench *bench)
{
	for (int i = 0; i < 64; ++i) {
		rte_pktmbuf_adj(bench->pkts[i], headers_length);
	}
}

static void inside_to_vxlan(int argc, char **argv, int type)
{
	struct pg_error *error = NULL;
	struct pg_brick *vtep;
	struct pg_bench bench;
	struct pg_bench_stats stats;
	struct pg_brick *inside_nop;
	struct ether_addr mac1 = {{0x52,0x54,0x00,0x12,0x34,0x11}};
	struct ether_addr mac2 = {{0x52,0x54,0x00,0x12,0x34,0x21}};
	struct ether_addr mac3 = {{0x52,0x54,0x00,0x12,0x34,0x31}};
	uint32_t len;

	if (type == AF_INET) {
		g_assert(!pg_bench_init(&bench, "vtep inside to vxlan 4",
					argc, argv, &error));
		vtep = pg_vtep_new("vtep", 1, PG_EAST_SIDE,
				   inet_addr("192.168.0.1"),
				   mac3, PG_VTEP_DST_PORT,
				   PG_VTEP_ALL_OPTI, &error);
		headers_length = sizeof(struct headers4);
	} else {
		uint8_t ip[16];

		headers_length = sizeof(struct headers6);
		g_assert(!pg_bench_init(&bench, "vtep inside to vxlan 6",
					argc, argv, &error));
		pg_ip_from_str(ip, "1::0.1");
		vtep = pg_vtep_new("vtep", 1, PG_EAST_SIDE, ip,
				   mac3, PG_VTEP_DST_PORT,
				   PG_VTEP_ALL_OPTI, &error);
	}
	g_assert(!error);

	inside_nop = pg_nop_new("nop-input", &error);
	bench.input_brick = inside_nop;
	bench.input_side = PG_WEST_SIDE;
	bench.output_brick = vtep;
	bench.output_side = PG_EAST_SIDE;
	bench.output_poll = false;
	bench.max_burst_cnt = 3000000;
	bench.count_brick = pg_nop_new("nop-inside", &error);
	bench.post_burst_op = remove_vtep_hdr;
	g_assert(!error);
	pg_brick_link(inside_nop, vtep, &error);
	g_assert(!error);
	pg_brick_link(vtep, bench.count_brick, &error);
	g_assert(!error);
	if (type == AF_INET) {
		pg_vtep_add_vni(vtep, inside_nop, 0,
				inet_addr("224.0.0.5"), &error);
	} else {
		uint8_t ip[16];

		pg_ip_from_str(ip, "ff00::3:1");
		pg_vtep_add_vni(vtep, inside_nop, 0,
				ip, &error);
	}
	g_assert(!error);
	bench.pkts_nb = 64;
	bench.pkts_mask = pg_mask_firsts(64);
	bench.pkts = pg_packets_create(bench.pkts_mask);
	bench.pkts = pg_packets_append_ether(
		bench.pkts,
		bench.pkts_mask,
		&mac1, &mac2,
		ETHER_TYPE_IPv4);
	len = 1356;
	pg_packets_append_ipv4(
		bench.pkts,
		bench.pkts_mask,
		0x000000EE, 0x000000CC,
		len + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr), 17);
	bench.pkts = pg_packets_append_udp(
		bench.pkts,
		bench.pkts_mask,
		1000, PG_VTEP_DST_PORT, len);
	bench.pkts = pg_packets_append_blank(bench.pkts, bench.pkts_mask, len);
	bench.brick_full_burst = 1;

	//g_assert(pg_bench_run(&bench, &stats, &error));
	pg_bench_run(&bench, &stats, &error);
	pg_error_print(error);
	g_assert(!pg_error_is_set(&error));

	pg_bench_print(&stats);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(vtep);
	pg_brick_destroy(inside_nop);
	pg_brick_destroy(bench.count_brick);
}

char vxlan_hdr[sizeof(struct headers6) + 1];

static void add_vtep_hdr(struct pg_bench *bench)
{
	bench->pkts = pg_packets_prepend_buf(
		bench->pkts,
		bench->pkts_mask,
		vxlan_hdr,
		headers_length);
}

static void vxlan_to_inside(int flags, const char *title,
			    int argc, char **argv, int type)
{
	struct pg_error *error = NULL;
	struct pg_brick *vtep;
	struct pg_bench bench;
	struct pg_bench_stats stats;
	struct pg_brick *outside_nop;
	struct ether_addr mac3 = {{0x52,0x54,0x00,0x12,0x34,0x31}};
	struct ether_addr mac4 = {{0x52,0x54,0x00,0x12,0x34,0x41}};
	static struct ether_addr mac_vtep = {{0xb0,0xb1,0xb2,0xb3,0xb4,0xb5}};
	uint32_t len;

	if (type == AF_INET) {
		headers_length = sizeof(struct headers4);
		vtep = pg_vtep_new("vtep", 1, PG_WEST_SIDE, 0x000000EE,
				   mac_vtep, PG_VTEP_DST_PORT, flags, &error);
	} else {
		uint8_t ip[16];

		headers_length = sizeof(struct headers6);
		pg_ip_from_str(ip, "1::1");
		vtep = pg_vtep_new("vtep", 1, PG_WEST_SIDE, ip,
				   mac_vtep, PG_VTEP_DST_PORT, flags, &error);
	}
	g_assert(!error);

	g_assert(!pg_bench_init(&bench, title, argc, argv, &error));
	outside_nop = pg_nop_new("nop-outside", &error);
	bench.input_brick = outside_nop;
	bench.input_side = PG_WEST_SIDE;
	bench.output_brick = vtep;
	bench.output_side = PG_EAST_SIDE;
	bench.output_poll = false;
	bench.max_burst_cnt = 1000000;
	bench.count_brick = pg_nop_new("nop-bench", &error);
	if (flags & PG_VTEP_NO_COPY)
		bench.post_burst_op = add_vtep_hdr;
	g_assert(!error);
	pg_brick_link(outside_nop, vtep, &error);
	g_assert(!error);
	pg_brick_link(vtep, bench.count_brick, &error);
	g_assert(!error);
	if (type == AF_INET) {
		pg_vtep_add_vni(vtep, bench.count_brick, 1,
				inet_addr("224.0.0.1"), &error);
		g_assert(!pg_error_is_set(&error));
	} else {
		uint8_t ip[16];

		pg_ip_from_str(ip, "ff00::1");
		pg_vtep_add_vni(vtep, bench.count_brick, 1, ip, &error);
		g_assert(!pg_error_is_set(&error));
	}
		if (pg_vtep_add_mac(vtep, 1, &mac4, &error) < 0)
			pg_error_print(error);
		g_assert(!pg_error_is_set(&error));
		if (pg_vtep_add_mac(vtep, 1, &mac3, &error) < 0)
			pg_error_print(error);
		g_assert(!pg_error_is_set(&error));

	bench.pkts_nb = 64;
	bench.pkts_mask = pg_mask_firsts(64);
	bench.pkts = pg_packets_create(bench.pkts_mask);
	bench.pkts = pg_packets_append_ether(
		bench.pkts,
		bench.pkts_mask,
		&mac_vtep, &mac_vtep,
		type == AF_INET ? ETHER_TYPE_IPv4 : ETHER_TYPE_IPv6);
	bench.brick_full_burst = 1;
	len = headers_length + 1400;
	if (type == AF_INET) {
		pg_packets_append_ipv4(
			bench.pkts,
			bench.pkts_mask,
			0x000000EE, 0x000000CC, len, 17);
	} else {
		uint8_t ip_src[16];
		uint8_t ip_dst[16];
		pg_ip_from_str(ip_src, "1::1");
		pg_ip_from_str(ip_dst, "1::2");
		
		pg_packets_append_ipv6(
			bench.pkts,
			bench.pkts_mask,
			ip_src, ip_dst, len, 17);
	}
	bench.pkts = pg_packets_append_udp(
		bench.pkts,
		bench.pkts_mask,
		1000, PG_VTEP_DST_PORT, 1400);
	pg_packets_append_vxlan(bench.pkts, bench.pkts_mask, 1);
	bench.pkts = pg_packets_append_ether(
		bench.pkts,
		bench.pkts_mask,
		&mac3, &mac4,
		ETHER_TYPE_IPv4);
	bench.pkts = pg_packets_append_blank(bench.pkts, bench.pkts_mask, 1400);
	memcpy(vxlan_hdr, rte_pktmbuf_mtod(bench.pkts[0], void *),
	       headers_length);
	vxlan_hdr[headers_length] = '\0';

	g_assert(pg_bench_run(&bench, &stats, &error) == 0);
	pg_bench_print(&stats);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(vtep);
	pg_brick_destroy(outside_nop);
	pg_brick_destroy(bench.count_brick);
}

void test_benchmark_vtep(int argc, char **argv)
{
	inside_to_vxlan(argc, argv, AF_INET);
	inside_to_vxlan(argc, argv, AF_INET6);
	vxlan_to_inside(PG_VTEP_ALL_OPTI, "vxlan 4 all opti",
			argc, argv, AF_INET);
	vxlan_to_inside(PG_VTEP_ALL_OPTI, "vxlan 6 all opti",
			argc, argv, AF_INET6);
	vxlan_to_inside(PG_VTEP_NO_COPY, "vxlan 4 bench no copy", argc,
			argv, AF_INET);
	vxlan_to_inside(PG_VTEP_NO_COPY, "vxlan 6 bench no copy", argc,
			argv, AF_INET6);
	vxlan_to_inside(PG_VTEP_NO_INNERMAC_CHECK,
			"vxlan 4 bench no innermac check",
			argc, argv, AF_INET);
	vxlan_to_inside(PG_VTEP_NO_INNERMAC_CHECK,
			"vxlan 6 bench no innermac check",
			argc, argv, AF_INET6);

	vxlan_to_inside(0, "vxlan 4 bench slow", argc, argv, AF_INET);
	vxlan_to_inside(0, "vxlan 6 bench slow", argc, argv, AF_INET6);
}

