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

#include <glib.h>

#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_eth_ring.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_ip_frag.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>

#include <packetgraph/graph.h>
#include <packetgraph/vtep.h>
#include <packetgraph/hub.h>
#include <packetgraph/nop.h>
#include <packetgraph/ip-fragment.h>

#include "brick-int.h"
#include "utils/tests.h"
#include "utils/mempool.h"
#include "utils/mac.h"
#include "utils/errors.h"
#include "utils/bitmask.h"
#include "utils/ip.h"
#include "utils/network.h"
#include "utils/malloc.h"
#include "packets.h"
#include "packetsgen.h"
#include "collect.h"

/* redeclar here for testing purpose,
 * theres stucture should not be use by user
 */

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)

struct headers {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	struct ipv4_hdr	 ipv4; /* define in rte_ip.h */
	struct udp_hdr	 udp; /* define in rte_udp.h */
	struct vxlan_hdr vxlan; /* define in rte_ether.h */
} __attribute__((__packed__));

/**
 * Composite structure of all the headers required to wrap a packet in VTEP
 */
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
	uint32_t ip;
	struct igmp_hdr igmp;
} __attribute__((__packed__));

struct mld_hdr {
	uint8_t type;
	uint8_t code;
	uint16_t cksum;
	uint32_t max_response_time;
	uint32_t reserved;
	union pg_ipv6_addr multicast_addr;
} __attribute__((__packed__));

struct multicast6_pkt {
	struct ether_hdr ethernet;
	struct ipv6_hdr ipv6;
	struct mld_hdr mld;
} __attribute__((__packed__));

struct vtep_state {
	struct pg_brick brick;
	struct ether_addr mac;		/* MAC address of the VTEP */
	union pg_ipv6_addr ip;		/* IP of the VTEP */
	enum pg_side output;		/* side the VTEP packets will go */
	uint16_t udp_dst_port;		/* UDP destination port */
	struct vtep_port *ports;
	rte_atomic16_t packet_id;	/* IP identification number */
	int flags;
	struct rte_mbuf *pkts[64];
};

#define NB_PKTS 20

static struct rte_mbuf **check_collector(struct pg_brick *collect,
					 struct rte_mbuf **(*sideFunc)(
						 struct pg_brick *brick,
						 uint64_t *pkts_mask,
						 struct pg_error **errp),
					 uint64_t nb)
{
	struct rte_mbuf **result_pkts;
	struct pg_error *error = NULL;
	uint64_t pkts_mask;

	result_pkts = sideFunc(collect, &pkts_mask, &error);
	pg_assert(!error);
	pg_assert(pkts_mask == pg_mask_firsts(nb));
	pg_assert((!!(intptr_t)result_pkts) == (!!nb));
	pg_error_assert_enable = 0;
	return result_pkts;
}

static void check_multicast_hdr(struct multicast_pkt *hdr, uint32_t ip_dst,
				uint32_t ip_src, struct ether_addr *src_addr,
				struct ether_addr *dst_addr)
{
	(void)hdr->igmp.checksum; /* cppcheck, don't speak please */
	g_assert(hdr->ethernet.ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4));
	g_assert(hdr->ipv4.version_ihl == 0x46);
	g_assert(hdr->ipv4.next_proto_id == 0x02);
	g_assert(hdr->ipv4.dst_addr == ip_dst);
	g_assert(hdr->ipv4.src_addr == ip_src);
	g_assert(hdr->igmp.type == 0x16);
	g_assert(hdr->igmp.maxRespTime == 0);
	g_assert(hdr->igmp.groupAddr == ip_dst);
	g_assert(is_same_ether_addr(&hdr->ethernet.d_addr, dst_addr));
	g_assert(is_same_ether_addr(&hdr->ethernet.s_addr, src_addr));
}

static void check_multicast6_hdr(struct multicast6_pkt *hdr)
{
	g_assert(hdr->ethernet.ether_type ==
		 rte_cpu_to_be_16(ETHER_TYPE_IPv6));
	g_assert(hdr->ipv6.vtc_flow == 0x60);
}

#define multicast_get_dst_addr(ip)					\
	(_Generic((ip),							\
		  uint32_t : multicast_get_dst4_addr,			\
		  union pg_ipv6_addr : multicast_get_dst6_addr) (ip))

static struct ether_addr multicast_get_dst4_addr(uint32_t ip)
{
	struct ether_addr dst;

	/* Forge dst mac addr */
	dst.addr_bytes[0] = 0x01;
	dst.addr_bytes[1] = 0x00;
	dst.addr_bytes[2] = 0x5e;
	dst.addr_bytes[3] = ((uint8_t *)&ip)[1] & 0x7f ; /* 16 - 24 */
	dst.addr_bytes[4] = ((uint8_t *)&ip)[2]; /* 9-16 */
	dst.addr_bytes[5] = ((uint8_t *)&ip)[3]; /* 1-8 */
	return dst;
}

static struct ether_addr multicast_get_dst6_addr(union pg_ipv6_addr ip)
{
	struct ether_addr dst;

	/* Forge dst mac addr */
	dst.addr_bytes[0] = 0x33;
	dst.addr_bytes[1] = 0x33;
	dst.addr_bytes[2] = ip.word8[12];
	dst.addr_bytes[3] = ip.word8[13];
	dst.addr_bytes[4] = ip.word8[14];
	dst.addr_bytes[5] = ip.word8[15];
	return dst;
}

static void l2_shorter(struct pg_brick *brick, enum pg_side output,
		       pg_packet_t **pkts, uint64_t *pkts_mask,
		       void *private_data)
{
	PG_PACKETS_FOREACH(pkts, *pkts_mask, pkt, it) {
		pg_packet_set_l2_len(pkt, sizeof(struct ether_hdr));
	}
}

static void test_vtep6_simple(void)
{
	struct ether_addr mac_dest = {{0xb0, 0xb1, 0xb2,
				      0xb3, 0xb4, 0xb5} };
	struct ether_addr mac_src = {{0xc0, 0xc1, 0xc2,
				      0xc3, 0xc4, 0xc5} };
	struct ether_addr multicast_mac1, multicast_mac2;
	struct pg_brick *vtep_west, *vtep_east;
	struct pg_brick *collect_west1, *collect_east1;
	struct pg_brick *collect_west2, *collect_east2;
	struct pg_brick *callback;
	struct pg_brick *collect_hub, *hub;
	struct pg_graph *graph;
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkts[NB_PKTS];
	struct rte_mbuf **result_pkts;
	struct pg_error *error = NULL;
	struct ether_hdr *tmp;
	union pg_ipv6_addr ip_vni1 = {.word16 = {0xff, 0, 0, 0, 0, 0, 0, 0x1} };
	union pg_ipv6_addr ip_vni2 = {.word16 = {0xff, 0, 0, 0, 0, 0, 0, 0x2} };
	int flag = 0;
	uint16_t i;

	vtep_west = pg_vtep_new_by_string("encapsulatron", 500, PG_EAST_SIDE,
					  "0::1", mac_src, PG_VTEP_DST_PORT,
					  flag, &error);
	g_assert(!error);
	g_assert(vtep_west);

	vtep_east = pg_vtep_new_by_string("decapsulatron", 500, PG_WEST_SIDE,
					  "0::2", mac_dest, PG_VTEP_DST_PORT,
					  flag, &error);
	g_assert(!error);
	g_assert(vtep_east);

	collect_east1 = pg_collect_new("collect-east1", &error);
	g_assert(!error);
	g_assert(collect_east1);
	collect_east2 = pg_collect_new("collect-east2", &error);
	g_assert(!error);
	g_assert(collect_east2);

	collect_west1 = pg_collect_new("collect-west1", &error);
	g_assert(!error);
	g_assert(collect_west1);
	collect_west2 = pg_collect_new("collect-west2", &error);
	g_assert(!error);
	g_assert(collect_west2);
	collect_hub = pg_collect_new("collect-hub", &error);
	g_assert(!error);
	g_assert(collect_hub);
	hub = pg_hub_new("hub", 3, 3, &error);
	g_assert(!error);
	g_assert(hub);
	callback = pg_user_dipole_new("call me", l2_shorter, NULL, &error);
	g_assert(!error);
	g_assert(callback);

	/*
	 * Here is an ascii graph of the links:
	 * CE = collect_east
	 * CW = collect_west
	 * CH = collect_hub
	 * VXE = vtep_east
	 * VXW = vtep_west
	 * H = hub
	 *
	 * [CW1] -\		    /- [CE1]
	 *	 [VXW] -- [H] -- [VXE]
	 * [CW2] -/         \-[CH]  \- [CE2]
	 */
	pg_brick_chained_links(&error, collect_west1, vtep_west, hub,
			       callback, vtep_east, collect_east1);
	g_assert(!error);
	pg_brick_link(collect_west2, vtep_west, &error);
	g_assert(!error);
	pg_brick_link(hub, collect_hub, &error);
	g_assert(!error);
	pg_brick_link(vtep_east, collect_east2, &error);
	g_assert(!error);


	graph = pg_graph_new("big dad", collect_west1, &error);
	g_assert(!error);

	pg_scan_ether_addr(&multicast_mac1, "01:00:5e:00:00:05");
	pg_scan_ether_addr(&multicast_mac2, "01:00:5e:00:00:06");

	pg_vtep6_add_vni(vtep_east, collect_east1, 0,
			 ip_vni1.word8, &error);
	g_assert(!error);
	pg_error_make_ctx(1);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get, 1);
	check_multicast6_hdr(rte_pktmbuf_mtod(result_pkts[0],
					      struct multicast6_pkt *));

	pg_vtep6_add_vni(vtep_east, collect_east2, 1,
			 ip_vni2.word8, &error);
	g_assert(!error);
	pg_error_make_ctx(1);
	check_collector(collect_hub, pg_brick_west_burst_get, 1);

	pg_vtep6_add_vni(vtep_west, collect_west1, 0,
			 ip_vni1.word8, &error);
	g_assert(!error);
	pg_error_make_ctx(1);
	check_collector(collect_hub, pg_brick_west_burst_get, 1);

	pg_vtep6_add_vni(vtep_west, collect_west2, 1,
			 ip_vni2.word8, &error);
	g_assert(!error);
	pg_error_make_ctx(1);
	check_collector(collect_hub, pg_brick_west_burst_get, 1);

	/* set mac */
	mac_src.addr_bytes[0] = 0xf0;
	mac_src.addr_bytes[1] = 0xf1;
	mac_src.addr_bytes[2] = 0xf2;
	mac_src.addr_bytes[3] = 0xf3;
	mac_src.addr_bytes[4] = 0xf4;
	mac_src.addr_bytes[5] = 0xf5;

	mac_dest.addr_bytes[0] = 0xe0;
	mac_dest.addr_bytes[1] = 0xe1;
	mac_dest.addr_bytes[2] = 0xe2;
	mac_dest.addr_bytes[3] = 0xe3;
	mac_dest.addr_bytes[4] = 0xe4;
	mac_dest.addr_bytes[5] = 0xe5;

	/* alloc packets */
	for (i = 0; i < NB_PKTS; i++) {
		char buf[34];

		pkts[i] = rte_pktmbuf_alloc(mp);
		g_assert(pkts[i]);
		pkts[i]->l2_len = sizeof(struct ether_hdr);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
				 pg_printable_mac(&mac_src, buf),
				 "FF:FF:FF:FF:FF:FF");
		pg_get_ether_addrs(pkts[i], &tmp);
		g_assert(is_same_ether_addr(&tmp->s_addr, &mac_src));
	}

	/* burst */
	pg_brick_burst_to_west(vtep_east, 0, pkts,
			       pg_mask_firsts(NB_PKTS), &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);

	pg_error_make_ctx(1);

	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get,
				      NB_PKTS);

	/* check the packets have been recive by collect_west1 */
	pg_error_make_ctx(1);
	check_collector(collect_west1, pg_brick_east_burst_get, NB_PKTS);

	/* check no packets have been recive by collect_west2 */
	pg_error_make_ctx(1);
	check_collector(collect_west2, pg_brick_east_burst_get, 0);

	for (i = 0; i < NB_PKTS; i++) {
		char buf[32];
		char buf2[32];
		struct headers6 *hdr;
		struct ether_addr mac_multicast =
			multicast_get_dst_addr(ip_vni1);

		hdr = rte_pktmbuf_mtod(result_pkts[i],
				       struct headers6 *);
		g_assert(is_same_ether_addr(&hdr->ethernet.d_addr,
					    &mac_multicast));
		g_assert(pg_ip_is_same(hdr->ipv6.dst_addr, &ip_vni1));
		pg_set_mac_addrs(pkts[i],
				 pg_printable_mac(&mac_dest, buf),
				 pg_printable_mac(&mac_src, buf2));
		pg_get_ether_addrs(pkts[i], &tmp);
		g_assert(is_same_ether_addr(&tmp->s_addr, &mac_dest));
		g_assert(is_same_ether_addr(&tmp->d_addr, &mac_src));
		g_assert(!hdr->udp.dgram_cksum);
	}

	pg_brick_burst_to_east(vtep_west, 0, pkts,
			       pg_mask_firsts(NB_PKTS), &error);
	g_assert(!error);

	/* check no packets have been recive by collect_west2 */
	pg_error_make_ctx(1);
	check_collector(collect_east2, pg_brick_west_burst_get,
			0);

	/* check the packets have been recive by collect_west1 */
	pg_error_make_ctx(1);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get,
				      NB_PKTS);

	pg_error_make_ctx(1);
	check_collector(collect_east1, pg_brick_west_burst_get,
			NB_PKTS);

	for (i = 0; i < NB_PKTS; i++) {
		struct headers6 *hdr;
		uint8_t ipp[16];

		hdr = rte_pktmbuf_mtod(result_pkts[i], struct headers6 *);
		g_assert(is_same_ether_addr(&hdr->ethernet.d_addr,
					    pg_vtep_get_mac(vtep_east)));
		pg_ip_from_str(ipp, "0::2");
		g_assert(pg_ip_is_same(hdr->ipv6.dst_addr, ipp));
	}

	for (i = 0; i < NB_PKTS; i++) {
		pg_get_ether_addrs(pkts[i], &tmp);
		tmp->s_addr.addr_bytes[3] = 0xc0;
		tmp->d_addr.addr_bytes[3] = 0xc1;
	}
	pg_malloc_should_fail = 1;

	g_assert(pg_brick_burst_to_east(vtep_west, 0, pkts,
					pg_mask_firsts(NB_PKTS), &error) < 0);

	g_assert(error && error->err_no == ENOMEM);
	pg_error_free(error);
	error = NULL;
	pg_malloc_should_fail = 0;

	g_assert(!pg_brick_burst_to_east(vtep_west, 0, pkts,
					 pg_mask_firsts(NB_PKTS - 1), &error));

	g_assert(!error);
	/* As tomino used to say: kill'em all */
	pg_graph_destroy(graph);
}

static void test_vtep_simple_internal(int flag)
{
	struct ether_addr mac_dest = {{0xb0, 0xb1, 0xb2,
				      0xb3, 0xb4, 0xb5} };
	struct ether_addr mac_src = {{0xc0, 0xc1, 0xc2,
				      0xc3, 0xc4, 0xc5} };
	struct ether_addr multicast_mac1, multicast_mac2;
	struct pg_brick *vtep_west, *vtep_east;
	struct pg_brick *collect_west1, *collect_east1;
	struct pg_brick *collect_west2, *collect_east2;
	struct pg_brick *collect_hub;
	struct pg_brick *callback;
	struct pg_brick *hub;
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkts[NB_PKTS];
	struct rte_mbuf **result_pkts;
	struct pg_error *error = NULL;
	struct ether_hdr *tmp;
	uint16_t i;

	/* for testing purpose this vtep ip is 1*/
	vtep_west = pg_vtep_new_by_string("vtep", 500, PG_EAST_SIDE,
					  "1.0.0.0", mac_src,
					  PG_VTEP_DST_PORT, flag, &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(vtep_west);

	/* for testing purpose this vtep ip is 2*/
	vtep_east = pg_vtep_new("vtep", 500, PG_WEST_SIDE, 2, mac_dest,
				PG_VTEP_DST_PORT, flag, &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(vtep_east);

	collect_east1 = pg_collect_new("collect-east1", &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(collect_east1);
	collect_east2 = pg_collect_new("collect-east2", &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(collect_east2);

	collect_west1 = pg_collect_new("collect-west1", &error);
	g_assert(!error);
	g_assert(collect_west1);
	collect_west2 = pg_collect_new("collect-west2", &error);
	g_assert(!error);
	g_assert(collect_west2);
	collect_hub = pg_collect_new("collect-hub", &error);
	g_assert(!error);
	g_assert(collect_hub);
	hub = pg_hub_new("hub", 3, 3, &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(hub);
	callback = pg_user_dipole_new("call me", l2_shorter, NULL, &error);
	g_assert(!error);
	g_assert(callback);

	/*
	 * here is an ascii graph of the links:
	 * CE = collect_east
	 * Cw = collect_west
	 * CH = collect_hub
	 * VXE = vtep_east
	 * VXW = vtep_west
	 * H = hub
	 *
	 * [CW1] -\		    /- [CE1]
	 *	 [VXW] -- [H] -- [VXE]
	 * [CW2] -/         \-[CH]  \- [CE2]
	 */
	pg_brick_link(collect_west1, vtep_west, &error);
	g_assert(!error);
	pg_brick_link(collect_west2, vtep_west, &error);
	g_assert(!error);
	pg_brick_link(vtep_west, hub, &error);
	g_assert(!error);

	pg_brick_link(hub, collect_hub, &error);
	g_assert(!error);

	pg_brick_chained_links(&error, hub, callback, vtep_east);
	g_assert(!error);
	pg_brick_link(vtep_east, collect_east1, &error);
	g_assert(!error);
	pg_brick_link(vtep_east, collect_east2, &error);
	g_assert(!error);

	pg_scan_ether_addr(&multicast_mac1, "01:00:5e:00:00:05");
	pg_scan_ether_addr(&multicast_mac2, "01:00:5e:00:00:06");
	/* configure VNI */
	pg_vtep_add_vni(vtep_east, collect_east1, 0,
		      inet_addr("224.0.0.5"), &error);
	g_assert(!error);

	pg_error_make_ctx(1);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.5"), 2, &mac_dest,
			    &multicast_mac1);

	pg_vtep_add_vni(vtep_east, collect_east2, 1,
		      inet_addr("224.0.0.6"), &error);
	g_assert(!error);
	pg_error_make_ctx(1);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.6"), 2, &mac_dest,
			    &multicast_mac2);

	pg_vtep_add_vni(vtep_west, collect_west1, 0,
		      inet_addr("224.0.0.5"), &error);
	g_assert(!error);
	pg_error_make_ctx(1);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.5"), 1, &mac_src,
			    &multicast_mac1);

	pg_vtep_add_vni(vtep_west, collect_west2, 1,
		      inet_addr("224.0.0.6"), &error);
	g_assert(!error);
	pg_error_make_ctx(1);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.6"), 1, &mac_src,
			    &multicast_mac2);

	/* set mac */
	mac_src.addr_bytes[0] = 0xf0;
	mac_src.addr_bytes[1] = 0xf1;
	mac_src.addr_bytes[2] = 0xf2;
	mac_src.addr_bytes[3] = 0xf3;
	mac_src.addr_bytes[4] = 0xf4;
	mac_src.addr_bytes[5] = 0xf5;

	mac_dest.addr_bytes[0] = 0xe0;
	mac_dest.addr_bytes[1] = 0xe1;
	mac_dest.addr_bytes[2] = 0xe2;
	mac_dest.addr_bytes[3] = 0xe3;
	mac_dest.addr_bytes[4] = 0xe4;
	mac_dest.addr_bytes[5] = 0xe5;

	/* alloc packets */
	for (i = 0; i < NB_PKTS; i++) {
		char buf[34];

		pkts[i] = rte_pktmbuf_alloc(mp);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		pg_set_mac_addrs(pkts[i],
			      pg_printable_mac(&mac_src, buf),
			      "FF:FF:FF:FF:FF:FF");
		pg_get_ether_addrs(pkts[i], &tmp);
		g_assert(is_same_ether_addr(&tmp->s_addr, &mac_src));
	}

	/* burst */
	pg_brick_burst_to_west(vtep_east, 0, pkts,
			    pg_mask_firsts(NB_PKTS), &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);

	/* check the packets have been recive by collect_west1 */
	pg_error_make_ctx(1);
	check_collector(collect_west1, pg_brick_east_burst_get, NB_PKTS);
	/* check no packets have been recive by collect_west2 */
	pg_error_make_ctx(1);
	check_collector(collect_west2, pg_brick_east_burst_get, 0);

	pg_error_make_ctx(1);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get,
				      NB_PKTS);

	if (!(flag & PG_VTEP_NO_COPY)) {
		for (i = 0; i < NB_PKTS; i++) {
			char buf[32];
			char buf2[32];
			struct headers *hdr;
			struct ether_addr mac_multicast =
				multicast_get_dst_addr(inet_addr("224.0.0.5"));

			hdr = rte_pktmbuf_mtod(result_pkts[i],
					       struct headers *);
			g_assert(is_same_ether_addr(&hdr->ethernet.d_addr,
						    &mac_multicast));
			g_assert(hdr->ipv4.dst_addr == inet_addr("224.0.0.5"));
			pg_set_mac_addrs(pkts[i],
					 pg_printable_mac(&mac_dest, buf),
					 pg_printable_mac(&mac_src, buf2));
			pg_get_ether_addrs(pkts[i], &tmp);
			g_assert(is_same_ether_addr(&tmp->s_addr, &mac_dest));
			g_assert(is_same_ether_addr(&tmp->d_addr, &mac_src));
			g_assert(!hdr->udp.dgram_cksum);
		}
	}
	pg_brick_burst_to_east(vtep_west, 0, pkts,
			    pg_mask_firsts(NB_PKTS), &error);
	g_assert(!error);

	/* check no packets have been recive by collect_west2 */
	pg_error_make_ctx(1);
	check_collector(collect_east2, pg_brick_west_burst_get,
				      0);

	/* check the packets have been recive by collect_west1 */
	pg_error_make_ctx(1);
	check_collector(collect_east1, pg_brick_west_burst_get,
				      NB_PKTS);

	pg_error_make_ctx(1);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get,
				      NB_PKTS);

	if (!(flag & PG_VTEP_NO_COPY)) {
		for (i = 0; i < NB_PKTS; i++) {
			struct headers *hdr;

			hdr = rte_pktmbuf_mtod(result_pkts[i],
					       struct headers *);
			g_assert(is_same_ether_addr(&hdr->ethernet.d_addr,
						    pg_vtep_get_mac(
								vtep_east)));
			g_assert(hdr->ipv4.dst_addr == 2);
		}
	}

	/* kill'em all */
	pg_brick_unlink(vtep_west, &error);
	g_assert(!error);
	pg_brick_decref(vtep_west, &error);
	g_assert(!error);
	pg_brick_unlink(vtep_east, &error);
	g_assert(!error);
	pg_brick_decref(vtep_east, &error);
	g_assert(!error);

	pg_brick_unlink(hub, &error);
	g_assert(!error);
	pg_brick_decref(hub, &error);
	g_assert(!error);

	pg_brick_unlink(collect_hub, &error);
	g_assert(!error);
	pg_brick_decref(collect_hub, &error);
	g_assert(!error);
	pg_brick_unlink(collect_west1, &error);
	g_assert(!error);
	pg_brick_decref(collect_west1, &error);
	g_assert(!error);
	pg_brick_unlink(collect_east1, &error);
	g_assert(!error);
	pg_brick_decref(collect_east1, &error);
	g_assert(!error);
	pg_brick_unlink(collect_west2, &error);
	g_assert(!error);
	pg_brick_decref(collect_west2, &error);
	g_assert(!error);
	pg_brick_unlink(collect_east2, &error);
	g_assert(!error);
	pg_brick_decref(collect_east2, &error);
	g_assert(!error);
	pg_brick_destroy(callback);
}

static void test_vtep_simple_no_flags(void)
{
	test_vtep_simple_internal(0);
}

static void test_vtep_simple_no_inner_check(void)
{
	test_vtep_simple_internal(PG_VTEP_NO_INNERMAC_CHECK);
}

static void test_vtep_simple_no_copy(void)
{
	test_vtep_simple_internal(PG_VTEP_NO_COPY);
}

static void test_vtep_simple_all_opti(void)
{
	test_vtep_simple_internal(PG_VTEP_NO_COPY);
}

#define NB_VNIS 20
#define NB_ITERATION 60

static void test_vtep_vnis(int flag)
{
	struct pg_brick *vtep, *collects[NB_VNIS];
	struct ether_addr mac1 = {{0xf1, 0xf2, 0xf3,
				   0xf4, 0xf5, 0xf6} };
	struct pg_error *error = NULL;
	struct rte_mbuf **pkts;
	uint64_t mask = pg_mask_firsts(64);
	uint32_t len;

	/*            / --- [collect   0] */
	/* [vtep] ---{------[collect X-1] */
	/*            \ --- [collect   X] */
	vtep = pg_vtep_new("vt", NB_VNIS, PG_WEST_SIDE,
			   15, mac1, PG_VTEP_DST_PORT, flag, &error);
	g_assert(!error);
	for (int i = 0; i < NB_VNIS; ++i) {
		collects[i] = pg_collect_new("collect", &error);
		g_assert(!error);
		pg_brick_link(vtep, collects[i], &error);
		g_assert(!error);
		pg_vtep_add_vni(vtep, collects[i], i,
				rte_cpu_to_be_32(0xe1e1e1e1 + i), &error);
		g_assert(!error);
	}
	pkts = pg_packets_create(mask);

	/* packet content */
	pg_packets_append_ether(pkts, mask, &mac1, &mac1, ETHER_TYPE_IPv4);
	pg_packets_append_blank(pkts, mask, 1400);
	for (uint32_t i = 0; i < NB_ITERATION; ++i) {
		uint64_t tmp_mask;

		len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) +
			sizeof(struct vxlan_hdr) +
			sizeof(struct ether_hdr) + 1400;

		for (uint32_t j = 0; j < NB_VNIS; ++j) {
			pg_brick_reset(collects[j], &error);
			g_assert(!error);
		}

		pg_packets_prepend_vxlan(pkts, mask & 0x0000ff,
					 (i - 1) % NB_VNIS);
		pg_packets_prepend_vxlan(pkts, mask & 0x00ff00,
					 i % NB_VNIS);
		pg_packets_prepend_vxlan(pkts, mask & 0xff0000,
					 (i + 1) % NB_VNIS);

		pg_packets_prepend_udp(pkts, mask, 1000, PG_VTEP_DST_PORT,
				       1400);
		pg_packets_prepend_ipv4(pkts, mask, 0x000000EE,
					0x000000CC, len, 17);
		pg_packets_prepend_ether(pkts, mask, &mac1, &mac1,
					 ETHER_TYPE_IPv4);

		pg_brick_burst_to_east(vtep, 0, pkts, mask, &error);
		g_assert(!error);
		for (uint32_t j = 0; j < NB_VNIS; ++j) {
			tmp_mask = 0;
			pg_brick_west_burst_get(collects[j],
						&tmp_mask, &error);

			/*
			 * for now if innermac check is activate, no packets
			 * should pass.
			 */
			if (!(flag & PG_VTEP_NO_INNERMAC_CHECK)) {
				g_assert(!tmp_mask);
				continue;
			}

			if (j == ((i - 1) % NB_VNIS))
				g_assert(tmp_mask == 0x0000ff);
			else if (j == i % NB_VNIS)
				g_assert(tmp_mask == 0x00ff00);
			else if (j == ((i + 1) % NB_VNIS))
				g_assert(tmp_mask == 0xff0000);
			else
				g_assert(!tmp_mask);
		}

		if (!(flag & PG_VTEP_NO_COPY)) {
			PG_FOREACH_BIT(mask, it) {
				rte_pktmbuf_adj(pkts[it],
						sizeof(struct ipv4_hdr) +
						sizeof(struct udp_hdr) +
						sizeof(struct vxlan_hdr) +
						sizeof(struct ether_hdr));
			}
		}
	}
	if (!(flag & PG_VTEP_NO_INNERMAC_CHECK))
		goto exit;

	g_assert(pg_brick_pkts_count_get(vtep, PG_EAST_SIDE) == 60 * 64);
	PG_FOREACH_BIT(mask, it) {
		rte_pktmbuf_adj(pkts[it], sizeof(struct ether_hdr));
	}
	mac1.addr_bytes[3] = 33;
	pg_packets_prepend_ether(pkts, mask, &mac1, &mac1, ETHER_TYPE_IPv4);
	pg_packets_prepend_vxlan(pkts, mask, 0);

	pg_packets_prepend_udp(pkts, mask, 1000, PG_VTEP_DST_PORT,
			       1400);
	pg_packets_prepend_ipv4(pkts, mask, 0x000000EE,
				0xE70000E7, len, 17);
	pg_packets_prepend_ether(pkts, mask, &mac1, &mac1,
				 ETHER_TYPE_IPv4);

	pg_malloc_should_fail = 1;
	g_assert(pg_brick_burst_to_east(vtep, 0, pkts, mask, &error) < 0);
	g_assert(error && error->err_no == ENOMEM);
	pg_error_free(error);
	error = NULL;
	pg_malloc_should_fail = 0;

	if (!(flag & PG_VTEP_NO_COPY)) {
		PG_FOREACH_BIT(mask, it) {
			rte_pktmbuf_adj(pkts[it],
					sizeof(struct ipv4_hdr) +
					sizeof(struct udp_hdr) +
					sizeof(struct vxlan_hdr) +
					sizeof(struct ether_hdr));
		}
	}
	pg_packets_prepend_vxlan(pkts, mask, 0);

	pg_packets_prepend_udp(pkts, mask, 1000, PG_VTEP_DST_PORT,
			       1400);
	pg_packets_prepend_ipv4(pkts, mask, 0x000000EE,
				0xE70000E7, len, 17);
	pg_packets_prepend_ether(pkts, mask, &mac1, &mac1,
				 ETHER_TYPE_IPv4);

	g_assert(!pg_brick_burst_to_east(vtep, 0, pkts, mask, &error));
	g_assert(!error);

exit:
	for (int i = 0; i < NB_VNIS; ++i)
		pg_brick_destroy(collects[i]);
	pg_brick_destroy(vtep);
	pg_packets_free(pkts, mask);
	free(pkts);
}


static void test_vtep_vnis_all_opti(void)
{
	test_vtep_vnis(PG_VTEP_ALL_OPTI);
}

static void test_vtep_vnis_no_copy(void)
{
	test_vtep_vnis(PG_VTEP_NO_COPY);
}

static void test_vtep_vnis_no_flags(void)
{
	test_vtep_vnis(0);
}

static void test_vtep_vnis_no_inner_check(void)
{
	test_vtep_vnis(PG_VTEP_NO_INNERMAC_CHECK);
}


#undef NB_ITERATION
#undef NB_VNIS

struct speed_test_headers {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	/* struct ipv4_hdr	 ipv4; */
	/* struct udp_hdr	 udp; */
	char *data;
} __attribute__((__packed__));

static void test_vtep_flood_encap_decap(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *nop_east, *pktgen_west, *vtep_east, *vtep_west;
	struct pg_brick *callback;
	struct ether_addr  multicast_mac1;
	struct ether_addr  multicast_mac2;
	uint64_t tot_send_pkts = 0;
	struct timeval start, end;
	struct rte_mbuf *pkts[64];

	for (int i = 0; i < NB_PKTS; ++i) {
		struct speed_test_headers *hdr;
		struct ether_addr src = {{0, 0, 0, 0, 0, 1} };
		struct ether_addr dst = {{0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0} };

		pkts[i] = rte_pktmbuf_alloc(pg_get_mempool());
		g_assert(pkts[i]);
		hdr = (struct speed_test_headers *) rte_pktmbuf_append(pkts[i],
					    sizeof(struct speed_test_headers));
		hdr->ethernet.ether_type = rte_cpu_to_be_16(0xCAFE);
		ether_addr_copy(&dst, &hdr->ethernet.d_addr);
		ether_addr_copy(&src, &hdr->ethernet.s_addr);
		strcpy(((char *)hdr + sizeof(struct ether_hdr)), "hello");
	}

	pg_scan_ether_addr(&multicast_mac1, "01:00:5e:00:00:05");
	pg_scan_ether_addr(&multicast_mac2, "01:00:5e:00:00:06");

	vtep_east = pg_vtep_new("vt-e", 1, PG_WEST_SIDE,
			     15, multicast_mac1, PG_VTEP_DST_PORT,
			     PG_VTEP_ALL_OPTI, &error);
	CHECK_ERROR(error);
	vtep_west = pg_vtep_new("vt-w", 1, PG_EAST_SIDE,
			     240, multicast_mac2, PG_VTEP_DST_PORT,
			     PG_VTEP_ALL_OPTI, &error);
	CHECK_ERROR(error);
	pktgen_west = pg_packetsgen_new("pkggen-west", 1, 1, PG_EAST_SIDE,
				     pkts, NB_PKTS, &error);
	CHECK_ERROR(error);
	nop_east = pg_nop_new("nop-east", &error);
	CHECK_ERROR(error);

	callback = pg_user_dipole_new("call me", l2_shorter, NULL, &error);
	g_assert(!error);
	g_assert(callback);

	pg_brick_chained_links(&error, pktgen_west, vtep_west,
			       callback, vtep_east, nop_east);
	CHECK_ERROR(error);

	pg_vtep_add_vni(vtep_west, pktgen_west, 0,
		     inet_addr("225.0.0.43"), &error);
	CHECK_ERROR(error);
	pg_vtep_add_vni(vtep_east, nop_east, 0,
		     inet_addr("225.0.0.43"), &error);
	CHECK_ERROR(error);

	gettimeofday(&end, 0);
	gettimeofday(&start, 0);

	/* subscribe east brick mac */
	do {
		struct rte_mbuf *tmp = rte_pktmbuf_alloc(pg_get_mempool());
		struct speed_test_headers *hdr;
		struct ether_addr src = {{0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0} };
		struct ether_addr dst = {{0, 0, 0, 0, 0, 1} };

		hdr = (struct speed_test_headers *) rte_pktmbuf_append(tmp,
					sizeof(struct speed_test_headers));
		hdr->ethernet.ether_type = rte_cpu_to_be_16(0xCAFE);
		ether_addr_copy(&dst, &hdr->ethernet.d_addr);
		ether_addr_copy(&src, &hdr->ethernet.s_addr);
		strcpy(((char *)hdr + sizeof(struct ether_hdr)), "hello");

		pg_brick_burst_to_west(nop_east, 0, &tmp, 1, &error);
	} while (0);

	while (end.tv_sec - start.tv_sec < 10) {
		uint16_t nb_send_pkts;

		for (int i = 0; i < 100; ++i) {
			g_assert(pg_brick_poll(pktgen_west,
					       &nb_send_pkts, &error) == 0);
			tot_send_pkts += nb_send_pkts;
		}
		gettimeofday(&end, 0);
	}

	g_assert(pg_brick_pkts_count_get(nop_east, PG_EAST_SIDE) ==
		 tot_send_pkts);
	pg_brick_destroy(nop_east);
	pg_brick_destroy(callback);
	pg_brick_destroy(pktgen_west);
	pg_brick_destroy(vtep_east);
	pg_brick_destroy(vtep_west);
	pg_packets_free(pkts, pg_mask_firsts(NB_PKTS));
}

static void test_vtep_flood_encapsulate(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *nop_east, *pktgen_west, *vtep_west;
	struct ether_addr  multicast_mac1;
	uint64_t tot_send_pkts = 0;
	struct timeval start, end;
	struct rte_mbuf *pkts[64];

	for (int i = 0; i < NB_PKTS; ++i) {
		struct speed_test_headers *hdr;
		struct ether_addr src = {{0, 0, 0, 0, 0, 1} };
		struct ether_addr dst = {{0xf, 0xf, 0xf, 0xf, 0xf, 0xf} };

		pkts[i] = rte_pktmbuf_alloc(pg_get_mempool());
		g_assert(pkts[i]);
		hdr = (struct speed_test_headers *) rte_pktmbuf_append(pkts[i],
					sizeof(struct speed_test_headers));
		hdr->ethernet.ether_type = rte_cpu_to_be_16(0xCAFE);
		ether_addr_copy(&dst, &hdr->ethernet.d_addr);
		ether_addr_copy(&src, &hdr->ethernet.s_addr);
		strcpy(((char *)hdr + sizeof(struct ether_hdr)), "hello");
	}

	pg_scan_ether_addr(&multicast_mac1, "01:00:5e:00:00:05");

	vtep_west = pg_vtep_new("vt-w", 1, PG_EAST_SIDE,
			     15, multicast_mac1, PG_VTEP_DST_PORT,
			     PG_VTEP_ALL_OPTI, &error);
	CHECK_ERROR(error);
	pktgen_west = pg_packetsgen_new("pkggen-west", 1, 1, PG_EAST_SIDE,
				     pkts, NB_PKTS, &error);
	CHECK_ERROR(error);
	nop_east = pg_nop_new("nop-east", &error);
	CHECK_ERROR(error);

	pg_brick_chained_links(&error, pktgen_west, vtep_west,
			       nop_east);
	CHECK_ERROR(error);

	pg_vtep_add_vni(vtep_west, pktgen_west, 0,
		     inet_addr("225.0.0.43"), &error);
	CHECK_ERROR(error);

	gettimeofday(&end, 0);
	gettimeofday(&start, 0);

	/* subscribe east brick mac */
	do {
		struct rte_mbuf *tmp = rte_pktmbuf_alloc(pg_get_mempool());
		struct speed_test_headers *hdr;
		struct ether_addr src = {{0xf, 0xf, 0xf, 0xf, 0xf, 0xf} };
		struct ether_addr dst = {{0, 0, 0, 0, 1} };

		hdr = (struct speed_test_headers *) rte_pktmbuf_append(tmp,
					sizeof(struct speed_test_headers));
		hdr->ethernet.ether_type = rte_cpu_to_be_16(0xCAFE);
		ether_addr_copy(&dst, &hdr->ethernet.d_addr);
		ether_addr_copy(&src, &hdr->ethernet.s_addr);
		strcpy(((char *)hdr + sizeof(struct ether_hdr)), "hello");

		pg_brick_burst_to_west(nop_east, 0, &tmp, 1, &error);
	} while (0);

	while (end.tv_sec - start.tv_sec < 5) {
		uint16_t nb_send_pkts;

		for (int i = 0; i < 100; ++i) {
			g_assert(pg_brick_poll(pktgen_west,
					       &nb_send_pkts, &error) == 0);
			tot_send_pkts += nb_send_pkts;
		}
		gettimeofday(&end, 0);
	}

	g_assert(pg_brick_pkts_count_get(nop_east, PG_EAST_SIDE) ==
		 tot_send_pkts + 1);
	pg_brick_destroy(nop_east);
	pg_brick_destroy(pktgen_west);
	pg_brick_destroy(vtep_west);
	pg_packets_free(pkts, pg_mask_firsts(NB_PKTS));
}

static void test_vtep_fragment_encap_decap(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *ip_fragment;
	struct pg_brick *vtep;
	struct pg_brick *collect_west;
	struct pg_brick *collect_east;
	struct rte_mbuf **pkts;
	uint64_t mask = pg_mask_firsts(64);
	struct ether_addr mac = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x11} };
	struct rte_mbuf **tmp_pkts;
	uint64_t tmp_mask;
	int len, pkt_len = 1500;

	collect_west = pg_collect_new("west col", &error);
	collect_east = pg_collect_new("east col", &error);
	ip_fragment = pg_ip_fragment_new("frag", PG_EAST_SIDE, 448, &error);
	vtep = pg_vtep_new("vtep", 20, PG_EAST_SIDE, 1, mac, PG_VTEP_DST_PORT,
			   PG_VTEP_ALL_OPTI, &error);

	g_assert(ip_fragment);
	g_assert(vtep);
	g_assert(collect_west);
	g_assert(collect_east);
	pg_brick_chained_links(&error,
			       collect_west, ip_fragment, vtep, collect_east);

	pg_vtep_add_vni(vtep, ip_fragment, 0,
		     inet_addr("225.0.0.43"), &error);

	pkts = pg_packets_create(mask);
	g_assert(pkts);
	g_assert(pg_packets_append_ether(pkts, mask, &mac,
					 &mac, ETHER_TYPE_IPv4));

	/* post_burst_op = append_blank; */
	len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + pkt_len;
	g_assert(pg_packets_append_ipv4(
			 pkts,
			 mask,
			 0x000000EE, 0x000000CC, len, 17));
	g_assert(pg_packets_append_blank(pkts, mask, pkt_len));
	PG_FOREACH_BIT(mask, it) {
		struct eth_ipv4_hdr *pkt_buf =
			rte_pktmbuf_mtod(pkts[it], struct eth_ipv4_hdr *);
		pkt_buf->ip.fragment_offset = 0;
	}
	g_assert(pg_brick_burst_to_east(ip_fragment, 0, pkts,
					mask, &error) >= 0);

	tmp_pkts = pg_brick_west_burst_get(collect_east, &tmp_mask, &error);
	g_assert(tmp_mask == pg_mask_firsts(4));
	PG_FOREACH_BIT(tmp_mask, it) {
		struct headers *hdr = rte_pktmbuf_mtod(tmp_pkts[it],
						       struct headers *);

		g_assert(tmp_pkts[it]->l2_len == sizeof(struct ether_hdr));
		g_assert(tmp_pkts[it]->l3_len == sizeof(struct ipv4_hdr));
		g_assert(tmp_pkts[it]->ol_flags & PKT_TX_UDP_CKSUM);
		hdr->ipv4.hdr_checksum = 0;
		hdr->udp.dgram_cksum = 0;
		hdr->udp.dgram_cksum =
			rte_ipv4_udptcp_cksum(&hdr->ipv4, &hdr->udp);
		hdr += 1;
		g_assert(!memcmp(&hdr->ethernet.s_addr, &mac, 6));
		g_assert(!memcmp(&hdr->ethernet.d_addr, &mac, 6));
		g_assert(rte_ipv4_frag_pkt_is_fragmented(&hdr->ipv4));
		tmp_pkts[it]->ol_flags = 0;
	}

	pg_brick_burst_to_west(vtep, 0, tmp_pkts, tmp_mask, &error);
	tmp_pkts = pg_brick_east_burst_get(collect_west, &tmp_mask, &error);
	g_assert(tmp_mask == 1);
	g_assert(tmp_pkts[0]->l2_len == sizeof(struct ether_hdr));
	g_assert(tmp_pkts[0]->l3_len == sizeof(struct ipv4_hdr));
	g_assert(tmp_pkts[0]->ol_flags & PKT_TX_UDP_CKSUM);
	g_assert(tmp_pkts[0]->ol_flags & PKT_TX_TCP_CKSUM);
	pg_packets_free(pkts, pg_mask_firsts(64));
	g_free(pkts);
	pg_brick_destroy(vtep);
	pg_brick_destroy(collect_west);
	pg_brick_destroy(collect_east);
	pg_brick_destroy(ip_fragment);
}

#undef NB_PKTS

int main(int argc, char **argv)
{
	int r;

	g_test_init(&argc, &argv, NULL);
	r = pg_start(argc, argv);
	g_assert(r >= 0);

	pg_test_add_func("/vtep6/simple/no-flags", test_vtep6_simple);
	pg_test_add_func("/vtep/simple/no-flags", test_vtep_simple_no_flags);
	pg_test_add_func("/vtep/simple/no-inner-check",
			test_vtep_simple_no_inner_check);
	pg_test_add_func("/vtep/simple/no-copy", test_vtep_simple_no_copy);
	pg_test_add_func("/vtep/simple/all-opti", test_vtep_simple_all_opti);

	pg_test_add_func("/vtep/vnis/all-opti", test_vtep_vnis_all_opti);
	pg_test_add_func("/vtep/vnis/no-copy", test_vtep_vnis_no_copy);
	pg_test_add_func("/vtep/vnis/no-flags", test_vtep_vnis_no_flags);
	pg_test_add_func("/vtep/vnis/no-inner-check",
			test_vtep_vnis_no_inner_check);

	pg_test_add_func("/vtep/flood/encapsulate",
			test_vtep_flood_encapsulate);
	pg_test_add_func("/vtep/flood/encap-decap",
			test_vtep_flood_encap_decap);

	pg_test_add_func("/vtep/fragmented/encap-decap",
			test_vtep_fragment_encap_decap);

	r = g_test_run();

	pg_stop();
	return r;
}
