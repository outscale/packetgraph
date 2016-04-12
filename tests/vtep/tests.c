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
#include <sys/time.h>

#include <packetgraph/vtep.h>
#include <packetgraph/hub.h>
#include <packetgraph/nop.h>
#include "brick.h"
#include "utils/mempool.h"
#include "utils/mac.h"
#include "utils/bitmask.h"
#include "packetsgen.h"
#include "collect.h"

void test_benchmark_vtep(void);

/* redeclar here for testing purpose,
 * theres stucture should not be use by user */

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
	g_assert(!error);
	g_assert(pkts_mask == pg_mask_firsts(nb));
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

static struct ether_addr multicast_get_dst_addr(uint32_t ip)
{
	struct ether_addr dst;


	/* Forge dst mac addr */
	dst.addr_bytes[0] = 0x01;
	dst.addr_bytes[1] = 0x00;
	dst.addr_bytes[2] = 0x5e;
	dst.addr_bytes[3] = ((uint8_t *)&ip)[1] & 0x7f ; // 16 - 24
	dst.addr_bytes[4] = ((uint8_t *)&ip)[2]; // 9-16
	dst.addr_bytes[5] = ((uint8_t *)&ip)[3]; // 1-8
	return dst;
}

static void test_vtep_simple(void)
{
	struct ether_addr mac_dest = {{0xb0, 0xb1, 0xb2,
				      0xb3, 0xb4, 0xb5} };
	struct ether_addr mac_src = {{0xc0, 0xc1, 0xc2,
				      0xc3, 0xc4, 0xc5} };
	struct ether_addr multicast_mac1, multicast_mac2;
	/*we're usin the same cfg for the collect and the hub*/
	struct pg_brick *vtep_west, *vtep_east;
	struct pg_brick *collect_west1, *collect_east1;
	struct pg_brick *collect_west2, *collect_east2;
	struct pg_brick *collect_hub;
	struct pg_brick *hub;
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkts[NB_PKTS];
	struct rte_mbuf **result_pkts;
	struct pg_error *error = NULL;
	struct ether_hdr *tmp;
	uint16_t i;

	/*For testing purpose this vtep ip is 1*/
	vtep_west = pg_vtep_new("vtep", 2, 1, EAST_SIDE, 1, mac_src, 0, &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(vtep_west);

	/*For testing purpose this vtep ip is 2*/
	vtep_east = pg_vtep_new("vtep", 1, 2, WEST_SIDE, 2, mac_dest, 0, &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(vtep_east);

 	collect_east1 = pg_collect_new("collect-east1", 1, 1, &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(collect_east1);
	collect_east2 = pg_collect_new("collect-east2", 1, 1, &error);
	if (error)
		pg_error_print(error);
	g_assert(!error);
	g_assert(collect_east2);

	collect_west1 = pg_collect_new("collect-west1", 1, 1, &error);
	g_assert(!error);
	g_assert(collect_west1);
	collect_west2 = pg_collect_new("collect-west2", 1, 1, &error);
	g_assert(!error);
	g_assert(collect_west2);
	collect_hub = pg_collect_new("collect-hub", 3, 3, &error);
	g_assert(!error);
	g_assert(collect_hub);
	hub = pg_hub_new("hub", 3, 3, &error);
	if (error)
		pg_error_print(error);
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
	pg_brick_link(collect_west1, vtep_west, &error);
	g_assert(!error);
	pg_brick_link(collect_west2, vtep_west, &error);
	g_assert(!error);
	pg_brick_link(vtep_west, hub, &error);
	g_assert(!error);

	pg_brick_link(hub, collect_hub, &error);
	g_assert(!error);

	pg_brick_link(hub, vtep_east, &error);
	g_assert(!error);
	pg_brick_link(vtep_east, collect_east1, &error);
	g_assert(!error);
	pg_brick_link(vtep_east, collect_east2, &error);
	g_assert(!error);

	pg_scan_ether_addr(&multicast_mac1, "01:00:5e:00:00:05");
	pg_scan_ether_addr(&multicast_mac2, "01:00:5e:00:00:06");
	/* Configure VNI */
	pg_vtep_add_vni(vtep_east, collect_east1, 0,
		      inet_addr("224.0.0.5"), &error);
	g_assert(!error);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.5"), 2, &mac_dest,
			    &multicast_mac1);

	pg_vtep_add_vni(vtep_east, collect_east2, 1,
		      inet_addr("224.0.0.6"), &error);
	g_assert(!error);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.6"), 2, &mac_dest,
			    &multicast_mac2);

	pg_vtep_add_vni(vtep_west, collect_west1, 0,
		      inet_addr("224.0.0.5"), &error);
	g_assert(!error);
	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get, 1);
	check_multicast_hdr(rte_pktmbuf_mtod(result_pkts[0],
					     struct multicast_pkt *),
			    inet_addr("224.0.0.5"), 1, &mac_src,
			    &multicast_mac1);

	pg_vtep_add_vni(vtep_west, collect_west2, 1,
		      inet_addr("224.0.0.6"), &error);
	g_assert(!error);
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

	/* Alloc packets */
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
	check_collector(collect_west1, pg_brick_east_burst_get, NB_PKTS);
	/* check no packets have been recive by collect_west2 */
	check_collector(collect_west2, pg_brick_east_burst_get, 0);

	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get,
				      NB_PKTS);

	for (i = 0; i < NB_PKTS; i++) {
		char buf[32];
		char buf2[32];
		struct headers *hdr;
		struct ether_addr mac_multicast = multicast_get_dst_addr(
			inet_addr("224.0.0.5"));

		hdr = rte_pktmbuf_mtod(result_pkts[i], struct headers *);
		g_assert(is_same_ether_addr(&hdr->ethernet.d_addr,
					    &mac_multicast));
		g_assert(hdr->ipv4.dst_addr == inet_addr("224.0.0.5"));
		pg_set_mac_addrs(pkts[i],
			      pg_printable_mac(&mac_dest, buf),
			      pg_printable_mac(&mac_src, buf2));
		pg_get_ether_addrs(pkts[i], &tmp);
		g_assert(is_same_ether_addr(&tmp->s_addr, &mac_dest));
		g_assert(is_same_ether_addr(&tmp->d_addr, &mac_src));
	}
	pg_brick_burst_to_east(vtep_west, 0, pkts,
			    pg_mask_firsts(NB_PKTS), &error);
	g_assert(!error);

	/* check no packets have been recive by collect_west2 */
	check_collector(collect_east2, pg_brick_west_burst_get,
				      0);

	/* check the packets have been recive by collect_west1 */
	check_collector(collect_east1, pg_brick_west_burst_get,
				      NB_PKTS);

	result_pkts = check_collector(collect_hub, pg_brick_west_burst_get,
				      NB_PKTS);

	for (i = 0; i < NB_PKTS; i++) {
		struct headers *hdr;

		hdr = rte_pktmbuf_mtod(result_pkts[i], struct headers *);
		g_assert(is_same_ether_addr(&hdr->ethernet.d_addr,
					    pg_vtep_get_mac(vtep_east)));
		g_assert(hdr->ipv4.dst_addr == 2);
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

}
#define PRINT_RESULT(test_name)	do {				\
		printf(test_name				\
		       "\ntotal pkts send: %lu\n"		\
		       "in %lu,%lusec\n"			\
		       "%lu,%luM pkts/s\n", tot_send_pkts,	\
		       end.tv_sec - start.tv_sec,		\
		       end.tv_usec - start.tv_usec,		\
		       tot_send_pkts / 5 / 1000000,			\
		       tot_send_pkts / 5 % 1000000);		\
	} while (0)

struct speed_test_headers {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	/* struct ipv4_hdr	 ipv4; */
	/* struct udp_hdr	 udp; */
	char *data;
} __attribute__((__packed__));


static void test_nop_speed(void)
{
 	struct pg_error *error = NULL;
	struct pg_brick *nop_east, *pktgen_west;

	/*TODO: pregenerate pkts for pkggen*/
	uint64_t tot_send_pkts = 0;
	struct timeval start, end;
	struct rte_mbuf *pkts[64];

	for (int i = 0; i < NB_PKTS; ++i) {
		struct speed_test_headers *hdr;
		struct ether_addr src = {{0,0,0,0,0,1}};
		struct ether_addr dst = {{0xf,0xf,0xf,0xf,0xf,0xf}};

		pkts[i] = rte_pktmbuf_alloc(pg_get_mempool());
		g_assert(pkts[i]);
		hdr = (struct speed_test_headers *) rte_pktmbuf_append(pkts[i],
								       sizeof(struct speed_test_headers));
		hdr->ethernet.ether_type = rte_cpu_to_be_16(0xCAFE);
		ether_addr_copy(&dst, &hdr->ethernet.d_addr);
		ether_addr_copy(&src, &hdr->ethernet.s_addr);
		strcpy(((char *)hdr + sizeof(struct ether_hdr)), "hello");
	}
	
	CHECK_ERROR(error);
	pktgen_west = pg_packetsgen_new("pkggen-west", 1, 1, EAST_SIDE,
				     pkts, NB_PKTS, &error);
	CHECK_ERROR(error);
	nop_east = pg_nop_new("nop-east", &error);
	CHECK_ERROR(error);	

	pg_brick_chained_links(&error, pktgen_west , nop_east);


	gettimeofday(&end, 0);
	gettimeofday(&start, 0);
	while (end.tv_sec - start.tv_sec < 5) {
		uint16_t nb_send_pkts;

		for (int i = 0; i < 100; ++i) {
			g_assert(pg_brick_poll(pktgen_west, &nb_send_pkts, &error) == 0);
			tot_send_pkts += nb_send_pkts;
		}
		gettimeofday(&end, 0);
	}

	g_assert(pg_brick_pkts_count_get(nop_east, EAST_SIDE) ==
		 tot_send_pkts);
	PRINT_RESULT("nop: ");
	pg_brick_destroy(nop_east);
	pg_brick_destroy(pktgen_west);
	
}

static void test_vtep_speed(void)
{
 	struct pg_error *error = NULL;
	struct pg_brick *nop_east, *pktgen_west, *vtep_east, *vtep_west;

	/*TODO: pregenerate pkts for pkggen*/
	struct ether_addr  multicast_mac1;
	struct ether_addr  multicast_mac2;
	uint64_t tot_send_pkts = 0;
	struct timeval start, end;
	struct rte_mbuf *pkts[64];

	for (int i = 0; i < NB_PKTS; ++i) {
		struct speed_test_headers *hdr;
		struct ether_addr src = {{0,0,0,0,0,1}};
		struct ether_addr dst = {{0xf0,0xf0,0xf0,0xf0,0xf0,0xf0}};

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

	vtep_east = pg_vtep_new("vt-e", 1, 1, WEST_SIDE,
			     15, multicast_mac1, ALL_OPTI, &error);
	CHECK_ERROR(error);
	vtep_west = pg_vtep_new("vt-w", 1, 1, EAST_SIDE,
			     240, multicast_mac2, ALL_OPTI, &error);
	CHECK_ERROR(error);
	pktgen_west = pg_packetsgen_new("pkggen-west", 1, 1, EAST_SIDE,
				     pkts, NB_PKTS, &error);
	CHECK_ERROR(error);
	nop_east = pg_nop_new("nop-east", &error);
	CHECK_ERROR(error);	

	pg_brick_chained_links(&error, pktgen_west , vtep_west,
			    vtep_east, nop_east);
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
		struct ether_addr src = {{0xf0,0xf0,0xf0,0xf0,0xf0,0xf0}};
		struct ether_addr dst = {{0,0,0,0,0,1}};

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

	g_assert(pg_brick_pkts_count_get(nop_east, EAST_SIDE) ==
		 tot_send_pkts);
	PRINT_RESULT("both dir");
	pg_brick_destroy(nop_east);
	pg_brick_destroy(pktgen_west);
	pg_brick_destroy(vtep_east);
	pg_brick_destroy(vtep_west);
}

static void test_vtep_vxlanise(void)
{
 	struct pg_error *error = NULL;
	struct pg_brick *nop_east, *pktgen_west, *vtep_west;

	/*TODO: pregenerate pkts for pkggen*/
	struct ether_addr  multicast_mac1;
	uint64_t tot_send_pkts = 0;
	struct timeval start, end;
	struct rte_mbuf *pkts[64];

	for (int i = 0; i < NB_PKTS; ++i) {
		struct speed_test_headers *hdr;
		struct ether_addr src = {{0,0,0,0,0,1}};
		struct ether_addr dst = {{0xf,0xf,0xf,0xf,0xf,0xf}};

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

	vtep_west = pg_vtep_new("vt-w", 1, 1, EAST_SIDE,
			     15, multicast_mac1, ALL_OPTI, &error);
	CHECK_ERROR(error);
	pktgen_west = pg_packetsgen_new("pkggen-west", 1, 1, EAST_SIDE,
				     pkts, NB_PKTS, &error);
	CHECK_ERROR(error);
	nop_east = pg_nop_new("nop-east", &error);
	CHECK_ERROR(error);	

	pg_brick_chained_links(&error, pktgen_west , vtep_west,
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
		struct ether_addr src = {{0xf,0xf,0xf,0xf,0xf,0xf}};
		struct ether_addr dst = {{0,0,0,0,0,1}};

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

	g_assert(pg_brick_pkts_count_get(nop_east, EAST_SIDE) ==
		 tot_send_pkts + 1);
	PRINT_RESULT("vxlanise");
	pg_brick_destroy(nop_east);
	pg_brick_destroy(pktgen_west);
	pg_brick_destroy(vtep_west);
}

#undef NB_PKTS

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int r;
	g_test_init(&argc, &argv, NULL);

	r = pg_start(argc, argv, &error);
	g_assert(r >= 0);
	g_assert(!error);

	g_test_add_func("/vtep/simple", test_vtep_simple);
	g_test_add_func("/nop/speed", test_nop_speed);
	g_test_add_func("/vtep/vxlanise/speed", test_vtep_vxlanise);
	g_test_add_func("/vtep/both/speed", test_vtep_speed);
	
	r = g_test_run();

	pg_stop();
	return r;
}
