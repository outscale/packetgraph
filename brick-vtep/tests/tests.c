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

#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_eth_ring.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <packetgraph/brick.h>
#include <packetgraph/vtep.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/mac.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/collect.h>
#include <packetgraph/hub.h>

/* redeclar here for testing purpose,
 * theres stucture should not be use by user */

struct headers {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	struct ipv4_hdr	 ipv4; /* define in rte_ip.h */
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

#define NB_PKTS 20

static struct rte_mbuf **check_collector(struct brick *collect,
					 struct rte_mbuf **(*sideFunc)(
						 struct brick *brick,
						 uint64_t *pkts_mask,
						 struct switch_error **errp),
					 uint64_t nb)
{
	struct rte_mbuf **result_pkts;
	struct switch_error *error = NULL;
	uint64_t pkts_mask;

	result_pkts = sideFunc(collect, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(nb));
	g_assert((!!(intptr_t)result_pkts) == (!!nb));
	return result_pkts;
}

static void check_multicast_hdr(struct multicast_pkt *hdr, uint32_t ip_dst,
				uint32_t ip_src, struct ether_addr *src_addr,
				struct ether_addr *dst_addr)
{
	(void)hdr->igmp.checksum; /* cppcheck, don't speak please */
	g_assert(hdr->ethernet.ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4));
	g_assert(hdr->ipv4.version_ihl == 0x45);
	g_assert(hdr->ipv4.next_proto_id == 0x02);
	g_assert(hdr->ipv4.dst_addr == ip_dst);
	g_assert(hdr->ipv4.src_addr == ip_src);
	g_assert(hdr->igmp.type == 0x16);
	g_assert(hdr->igmp.maxRespTime == 0);
	g_assert(hdr->igmp.groupAddr == ip_dst);
	g_assert(is_same_ether_addr(&hdr->ethernet.d_addr, dst_addr));
	g_assert(is_same_ether_addr(&hdr->ethernet.s_addr, src_addr));
}

static void test_vtep_simple(void)
{
	struct ether_addr mac_dest = {{0xb0, 0xb1, 0xb2,
				      0xb3, 0xb4, 0xb5} };
	struct ether_addr mac_src = {{0xc0, 0xc1, 0xc2,
				      0xc3, 0xc4, 0xc5} };
	struct ether_addr multicast_mac1, multicast_mac2;
	/*we're usin the same cfg for the collect and the hub*/
	struct brick *vtep_west, *vtep_east;
	struct brick *collect_west1, *collect_east1;
	struct brick *collect_west2, *collect_east2;
	struct brick *collect_hub;
	struct brick *hub;
	struct rte_mempool *mp = get_mempool();
	struct rte_mbuf *pkts[NB_PKTS];
	struct rte_mbuf **result_pkts;
	struct switch_error *error = NULL;
	struct ether_hdr *tmp;
	uint16_t i;

	/*For testing purpose this vtep ip is 1*/
	vtep_west = vtep_new("vtep", 2, 1, EAST_SIDE, 1, mac_src, &error);
	if (error)
		error_print(error);
	g_assert(!error);
	g_assert(vtep_west);

	/*For testing purpose this vtep ip is 2*/
	vtep_east = vtep_new("vtep", 1, 2, WEST_SIDE, 2, mac_dest, &error);
	if (error)
		error_print(error);
	g_assert(!error);
	g_assert(vtep_east);

 	collect_east1 = collect_new("collect-east1", 1, 1, &error);
	if (error)
		error_print(error);
	g_assert(!error);
	g_assert(collect_east1);
	collect_east2 = collect_new("collect-east2", 1, 1, &error);
	if (error)
		error_print(error);
	g_assert(!error);
	g_assert(collect_east2);

	collect_west1 = collect_new("collect-west1", 1, 1, &error);
	g_assert(!error);
	g_assert(collect_west1);
	collect_west2 = collect_new("collect-west2", 1, 1, &error);
	g_assert(!error);
	g_assert(collect_west2);
	collect_hub = collect_new("collect-hub", 3, 3, &error);
	g_assert(!error);
	g_assert(collect_hub);
	hub = hub_new("hub", 3, 3, &error);
	if (error)
		error_print(error);
	g_assert(!error);
	g_assert(hub);

	/*
	 * Here is an ascii graph of the links:
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
	brick_link(collect_west1, vtep_west, &error);
	g_assert(!error);
	brick_link(collect_west2, vtep_west, &error);
	g_assert(!error);
	brick_link(vtep_west, hub, &error);
	g_assert(!error);

	brick_link(hub, collect_hub, &error);
	g_assert(!error);

	brick_link(hub, vtep_east, &error);
	g_assert(!error);
	brick_link(vtep_east, collect_east1, &error);
	g_assert(!error);
	brick_link(vtep_east, collect_east2, &error);
	g_assert(!error);

	scan_ether_addr(&multicast_mac1, "10:5e:00:00:00:05");
	scan_ether_addr(&multicast_mac2, "10:5e:00:00:00:06");
	/* Configure VNI */
	vtep_add_vni(vtep_east, collect_east1, 0,
		      inet_addr("224.0.0.5"), &error);
	g_assert(!error);
	result_pkts = check_collector(collect_hub, brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.5"), 2, &mac_dest,
			    &multicast_mac1);

	vtep_add_vni(vtep_east, collect_east2, 1,
		      inet_addr("224.0.0.6"), &error);
	g_assert(!error);
	result_pkts = check_collector(collect_hub, brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.6"), 2, &mac_dest,
			    &multicast_mac2);

	vtep_add_vni(vtep_west, collect_west1, 0,
		      inet_addr("224.0.0.5"), &error);
	g_assert(!error);
	result_pkts = check_collector(collect_hub, brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.5"), 1, &mac_src,
			    &multicast_mac1);

	vtep_add_vni(vtep_west, collect_west2, 1,
		      inet_addr("224.0.0.6"), &error);
	g_assert(!error);
	result_pkts = check_collector(collect_hub, brick_west_burst_get, 1);
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

	/* Alloc packets */
	for (i = 0; i < NB_PKTS; i++) {
		char buf[34];

		pkts[i] = rte_pktmbuf_alloc(mp);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i],
			      printable_mac(&mac_src, buf),
			      "FF:FF:FF:FF:FF:FF");
		get_ether_addrs(pkts[i], &tmp);
		g_assert(is_same_ether_addr(&tmp->s_addr, &mac_src));
	}

	/* burst */
	brick_burst_to_west(vtep_east, 0, pkts, NB_PKTS,
			    mask_firsts(NB_PKTS), &error);
	if (error)
		error_print(error);
	g_assert(!error);

	/* check the packets have been recive by collect_west1 */
	check_collector(collect_west1, brick_east_burst_get, NB_PKTS);
	/* check no packets have been recive by collect_west2 */
	check_collector(collect_west2, brick_east_burst_get, 0);

	result_pkts = check_collector(collect_hub, brick_west_burst_get,
				      NB_PKTS);

	for (i = 0; i < NB_PKTS; i++) {
		char buf[32];
		char buf2[32];
		struct headers *hdr;
		struct ether_addr mac_multicast = {{1, 0, 0, 0, 0, 0} };

		hdr = rte_pktmbuf_mtod(result_pkts[i], struct headers *);
		g_assert(is_same_ether_addr(&hdr->ethernet.d_addr,
					    &mac_multicast));
		g_assert(rte_be_to_cpu_32(hdr->ipv4.dst_addr) ==
			 inet_addr("224.0.0.5"));
		set_mac_addrs(pkts[i],
			      printable_mac(&mac_dest, buf),
			      printable_mac(&mac_src, buf2));
		get_ether_addrs(pkts[i], &tmp);
		g_assert(is_same_ether_addr(&tmp->s_addr, &mac_dest));
		g_assert(is_same_ether_addr(&tmp->d_addr, &mac_src));
	}
	brick_burst_to_east(vtep_west, 0, pkts, NB_PKTS,
			    mask_firsts(NB_PKTS), &error);
	g_assert(!error);

	/* check no packets have been recive by collect_west2 */
	check_collector(collect_east2, brick_west_burst_get,
				      0);

	/* check the packets have been recive by collect_west1 */
	check_collector(collect_east1, brick_west_burst_get,
				      NB_PKTS);

	result_pkts = check_collector(collect_hub, brick_west_burst_get,
				      NB_PKTS);

	for (i = 0; i < NB_PKTS; i++) {
		struct headers *hdr;

		hdr = rte_pktmbuf_mtod(result_pkts[i], struct headers *);
		g_assert(is_same_ether_addr(&hdr->ethernet.d_addr,
					    vtep_get_mac(vtep_east)));
		g_assert(rte_be_to_cpu_32(hdr->ipv4.dst_addr) == 2);
	}

	/* kill'em all */
	brick_unlink(vtep_west, &error);
	g_assert(!error);
	brick_decref(vtep_west, &error);
	g_assert(!error);
	brick_unlink(vtep_east, &error);
	g_assert(!error);
	brick_decref(vtep_east, &error);
	g_assert(!error);

	brick_unlink(hub, &error);
	g_assert(!error);
	brick_decref(hub, &error);
	g_assert(!error);

	brick_unlink(collect_hub, &error);
	g_assert(!error);
	brick_decref(collect_hub, &error);
	g_assert(!error);
	brick_unlink(collect_west1, &error);
	g_assert(!error);
	brick_decref(collect_west1, &error);
	g_assert(!error);
	brick_unlink(collect_east1, &error);
	g_assert(!error);
	brick_decref(collect_east1, &error);
	g_assert(!error);
	brick_unlink(collect_west2, &error);
	g_assert(!error);
	brick_decref(collect_west2, &error);
	g_assert(!error);
	brick_unlink(collect_east2, &error);
	g_assert(!error);
	brick_decref(collect_east2, &error);
	g_assert(!error);

}

#undef NB_PKTS

static void test_vtep(void)
{
	g_test_add_func("/brick/vtep/simple",
			test_vtep_simple);
}

int main(int argc, char **argv)
{
	struct switch_error *error;
	int r;

	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	packetgraph_start(argc, argv, &error);
	g_assert(!error);

	test_vtep();
	r = g_test_run();

	packetgraph_stop();
	return r;
}
