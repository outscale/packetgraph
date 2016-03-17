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
#include <string.h>
#include <arpa/inet.h>
#include <rte_config.h>
#include <rte_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <packetgraph/packetgraph.h>
#include <packetgraph/print.h>
#include "brick.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "collect.h"
#include "packetsgen.h"

#define NB_PKTS 3

static struct rte_mbuf *build_ip_packet(void)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mp);
	uint16_t len = sizeof(struct ether_hdr) + sizeof(struct ip) + 6;
	struct ether_hdr *eth;
	struct ip *ip;
	uint8_t *payload_ip;

	pkt->pkt_len = len;
	pkt->data_len = len;
	pkt->nb_segs = 1;
	pkt->next = NULL;

	/* ethernet header */
	eth = rte_pktmbuf_mtod(pkt, struct ether_hdr*);
	memset(eth, 0, len);
	eth->ether_type = htobe16(ETHER_TYPE_IPv4);

	/* ipv4 header */
	ip = (struct ip *)(eth + 1);
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_off = 0;
	ip->ip_ttl = 64;

	/* FEAR ! this is CHAOS ! */
	ip->ip_p = 16;
	ip->ip_len = htons(sizeof(struct ip) + 1);
	ip->ip_src.s_addr = inet_addr("10.0.42.1");
	ip->ip_dst.s_addr = inet_addr("10.0.42.2");

	/* write some data */
	payload_ip = (uint8_t *)(ip + 1);
	payload_ip[0] = 0x40;
	payload_ip[1] = 0x41;
	payload_ip[2] = 0x42;
	payload_ip[3] = 0x43;
	payload_ip[4] = 0x44;
	payload_ip[5] = 0x45;
	return pkt;
}

static void build_packets(struct rte_mbuf *packets[NB_PKTS])
{
	int i;

	for (i = 0; i < NB_PKTS; i++)
		packets[i] = build_ip_packet();
}

static void test_print_simple(void)
{
	int i;
	struct pg_brick *gen, *print, *col;
	struct pg_error *error = NULL;
	struct rte_mbuf *packets[NB_PKTS];
	struct rte_mbuf **pkts;
	uint64_t pkts_mask;

	build_packets(packets);
	gen = pg_packetsgen_new("gen", 1, 1, EAST_SIDE, packets, NB_PKTS,
				&error);
	g_assert(!error);
	print = pg_print_new("My print", 1, 1, NULL, PG_PRINT_FLAG_MAX, NULL,
			     &error);
	g_assert(!error);
	col = pg_collect_new("col", 1, 1, &error);
	g_assert(!error);

	pg_brick_link(gen, print, &error);
	g_assert(!error);
	pg_brick_link(print, col, &error);
	g_assert(!error);

	pg_brick_burst_to_east(gen, 0, packets,
			       pg_mask_firsts(NB_PKTS), &error);
	g_assert(!error);

	pkts = pg_brick_west_burst_get(col, &pkts_mask, &error);
	g_assert(!error);
	pg_packets_free(pkts, pg_mask_firsts(NB_PKTS));

	pg_brick_unlink(gen, &error);
	g_assert(!error);
	pg_brick_unlink(print, &error);
	g_assert(!error);
	pg_brick_unlink(col, &error);
	g_assert(!error);
	pg_brick_destroy(gen);
	pg_brick_destroy(print);
	pg_brick_destroy(col);
	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(packets[i]);
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int r;

	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	pg_start(argc, argv, &error);
	g_assert(!error);

	g_test_add_func("/brick/print/simple", test_print_simple);

	r = g_test_run();

	pg_stop();
	return r;
}
