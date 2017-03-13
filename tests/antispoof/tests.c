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
#include <string.h>
#include <rte_config.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include <packetgraph/antispoof.h>
#include "packetsgen.h"
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "collect.h"
#include "fail.h"
#include "utils/mac.h"
#include "utils/ip.h"

static struct rte_mbuf *build_packet(const unsigned char *data, size_t len)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mp);
	void *packet;

	pkt->pkt_len = len;
	pkt->data_len = len;
	pkt->nb_segs = 1;
	pkt->next = NULL;

	packet = rte_pktmbuf_mtod(pkt, void*);
	memcpy(packet, data, len);
	return pkt;
}

static void test_antispoof_mac(void)
{
#	include "test-arp-gratuitous.c"
	const unsigned char *pkts[] = {pkt1};
	int pkts_size[] = {60};
	uint16_t pkts_nb = 1;
	struct ether_addr inside_mac;
	struct pg_brick *gen_west;
	struct pg_brick *antispoof;
	struct pg_brick *col_east;
	struct pg_error *error = NULL;
	uint16_t packet_count;
	uint16_t i;
	struct rte_mbuf *packet;
	uint64_t filtered_pkts_mask;

	/* only those packets should pass */
	pg_scan_ether_addr(&inside_mac, "01:23:45:67:89:ab");

	/* [generator>]--[antispoof]--[collector] */
	gen_west = pg_packetsgen_new("gen_west", 1, 1, PG_EAST_SIDE,
				     &packet, 1, &error);
	g_assert(!error);
	antispoof = pg_antispoof_new("antispoof", PG_EAST_SIDE,
				     &inside_mac, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	pg_brick_link(gen_west, antispoof, &error);
	g_assert(!error);
	pg_brick_link(antispoof, col_east, &error);
	g_assert(!error);

	/* replay traffic */
	for (i = 0; i < pkts_nb; i++) {
		packet = build_packet(pkts[i], pkts_size[i]);
		pg_brick_poll(gen_west, &packet_count, &error);
		g_assert(!error);
		g_assert(packet_count == 1);
		pg_brick_west_burst_get(col_east, &filtered_pkts_mask, &error);
		g_assert(!error);
		g_assert(pg_mask_count(filtered_pkts_mask) == 0);
		rte_pktmbuf_free(packet);
	}
	pg_brick_destroy(gen_west);
	pg_brick_destroy(antispoof);
	pg_brick_destroy(col_east);
}

static void test_antispoof_rarp(void)
{
#	include "test-rarp.c"
	const unsigned char *pkts[] = {pkt1};
	int pkts_size[] = {15};
	uint16_t pkts_nb = 1;
	struct ether_addr inside_mac;
	struct pg_brick *gen_west;
	struct pg_brick *antispoof;
	struct pg_brick *col_east;
	struct pg_error *error = NULL;
	uint16_t packet_count;
	uint16_t i;
	struct rte_mbuf *packet;
	uint64_t filtered_pkts_mask;

	pg_scan_ether_addr(&inside_mac, "00:23:df:ff:c9:23");

	/* [generator>]--[antispoof]--[collector] */
	gen_west = pg_packetsgen_new("gen_west", 1, 1, PG_EAST_SIDE,
				     &packet, 1, &error);
	g_assert(!error);
	antispoof = pg_antispoof_new("antispoof", PG_EAST_SIDE,
				     &inside_mac, &error);
	pg_antispoof_arp_enable(antispoof);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	pg_brick_link(gen_west, antispoof, &error);
	g_assert(!error);
	pg_brick_link(antispoof, col_east, &error);
	g_assert(!error);

	/* replay traffic */
	for (i = 0; i < pkts_nb; i++) {
		packet = build_packet(pkts[i], pkts_size[i]);
		pg_brick_poll(gen_west, &packet_count, &error);
		g_assert(!error);
		g_assert(packet_count == 1);
		pg_brick_west_burst_get(col_east, &filtered_pkts_mask, &error);
		g_assert(!error);
		g_assert(pg_mask_count(filtered_pkts_mask) == 0);
		rte_pktmbuf_free(packet);
	}
	pg_brick_destroy(gen_west);
	pg_brick_destroy(antispoof);
	pg_brick_destroy(col_east);

}

static void test_antispoof_generic(const unsigned char **pkts,
				   int *pkts_size,
				   uint16_t pkts_nb,
				   struct ether_addr inside_mac,
				   uint32_t inside_ip)
{
	struct pg_brick *gen_west;
	struct pg_brick *antispoof;
	struct pg_brick *col_east;
	struct pg_error *error = NULL;
	uint16_t packet_count;
	uint16_t i;
	struct rte_mbuf *packet;
	uint64_t filtered_pkts_mask;
	struct rte_mbuf **filtered_pkts;

	/* [generator>]--[antispoof]--[collector] */
	gen_west = pg_packetsgen_new("gen_west", 1, 1, PG_EAST_SIDE,
				     &packet, 1, &error);
	g_assert(!error);
	antispoof = pg_antispoof_new("antispoof", PG_EAST_SIDE,
				     &inside_mac, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	pg_brick_link(gen_west, antispoof, &error);
	g_assert(!error);
	pg_brick_link(antispoof, col_east, &error);
	g_assert(!error);

#define REPLAY(pass) \
	for (i = 0; i < pkts_nb; i++) { \
		g_assert(pg_brick_reset(col_east, &error) >= 0); \
		g_assert(!error); \
		packet = build_packet(pkts[i], pkts_size[i]); \
		pg_brick_poll(gen_west, &packet_count, &error); \
		g_assert(!error); \
		g_assert(packet_count == 1); \
		filtered_pkts = pg_brick_west_burst_get(col_east, \
							&filtered_pkts_mask, \
							&error); \
		g_assert(!error); \
		g_assert(pg_mask_count(filtered_pkts_mask) == (pass)); \
		pg_packets_free(filtered_pkts, filtered_pkts_mask); \
		rte_pktmbuf_free(packet); \
	}

	/* enable ARP antispoof with the correct IP */
	pg_antispoof_arp_enable(antispoof);
	g_assert(!pg_antispoof_arp_add(antispoof, inside_ip, &error));
	g_assert(!error);
	REPLAY(1);

	/* remove IP, should not pass */
	g_assert(!pg_antispoof_arp_del(antispoof, inside_ip, &error));
	g_assert(!error);
	REPLAY(0);

	/* re-add other IP and original IP, should pass */
	g_assert(!pg_antispoof_arp_add(antispoof, inside_ip, &error));
	g_assert(!pg_antispoof_arp_add(antispoof, 42, &error));
	g_assert(!pg_antispoof_arp_add(antispoof, 51, &error));
	g_assert(!error);
	REPLAY(1);

	/* remove IP, should not pass */
	g_assert(!pg_antispoof_arp_del(antispoof, inside_ip, &error));
	g_assert(!error);
	REPLAY(0);

	/* re-add other IP and original IP, should pass */
	g_assert(!pg_antispoof_arp_del(antispoof, 42, &error));
	g_assert(!pg_antispoof_arp_del(antispoof, 51, &error));
	g_assert(!pg_antispoof_arp_add(antispoof, 42, &error));
	g_assert(!pg_antispoof_arp_add(antispoof, inside_ip, &error));
	g_assert(!pg_antispoof_arp_add(antispoof, 51, &error));
	g_assert(!error);
	REPLAY(1);

	/* remove IP, should not pass */
	g_assert(!pg_antispoof_arp_del(antispoof, inside_ip, &error));
	g_assert(!error);
	REPLAY(0);

	/* re-add other IP and original IP, should pass */
	g_assert(!pg_antispoof_arp_del(antispoof, 42, &error));
	g_assert(!pg_antispoof_arp_del(antispoof, 51, &error));
	g_assert(!pg_antispoof_arp_add(antispoof, 42, &error));
	g_assert(!pg_antispoof_arp_add(antispoof, 51, &error));
	g_assert(!pg_antispoof_arp_add(antispoof, inside_ip, &error));
	g_assert(!error);
	REPLAY(1);

	/* remove IP, should not pass */
	g_assert(!pg_antispoof_arp_del(antispoof, inside_ip, &error));
	g_assert(!error);
	REPLAY(0);

	/* re-remove IP, should have an error */
	g_assert(pg_antispoof_arp_del(antispoof, inside_ip, &error) == -1);
	g_assert(error);
	pg_error_free(error);
	error = NULL;

	/* disable arp antispoof, should pass */
	pg_antispoof_arp_disable(antispoof);
	REPLAY(1);

	/* enable arp antispoof again, should re-block */
	pg_antispoof_arp_enable(antispoof);
	REPLAY(0);

	/* reverse brick 'outside', packets must pass now */
	pg_brick_unlink(antispoof, &error);
	g_assert(!error);
	pg_brick_destroy(antispoof);
	antispoof = pg_antispoof_new("antispoof", PG_WEST_SIDE,
				     &inside_mac, &error);
	pg_antispoof_arp_enable(antispoof);
	g_assert(!pg_antispoof_arp_add(antispoof, inside_ip, &error));
	g_assert(!error);
	pg_brick_link(gen_west, antispoof, &error);
	g_assert(!error);
	pg_brick_link(antispoof, col_east, &error);
	g_assert(!error);
	REPLAY(1);
#undef REPLAY

	pg_brick_destroy(gen_west);
	pg_brick_destroy(antispoof);
	pg_brick_destroy(col_east);
}

static void test_pg_antispoof_arp_disable(void)
{
#	include "test-arp-request.c"
	const unsigned char *pkts[] = {pkt1};
	int pkts_size[] = {42};
	uint16_t pkts_nb = 1;
	struct ether_addr inside_mac;
	uint32_t inside_ip;
	struct pg_brick *gen_west;
	struct pg_brick *antispoof;
	struct pg_brick *col_east;
	struct pg_error *error = NULL;
	uint16_t packet_count;
	uint16_t i;
	struct rte_mbuf *packet;
	uint64_t filtered_pkts_mask;
	struct rte_mbuf **filtered_pkts;

	pg_scan_ether_addr(&inside_mac, "00:e0:81:d5:02:91");
	inside_ip = htobe32(IPv4(0, 0, 0, 42));

	/* [generator>]--[antispoof]--[collector] */
	gen_west = pg_packetsgen_new("gen_west", 1, 1, PG_EAST_SIDE,
				     &packet, 1, &error);
	g_assert(!error);
	antispoof = pg_antispoof_new("antispoof", PG_EAST_SIDE,
				     &inside_mac, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	pg_brick_link(gen_west, antispoof, &error);
	g_assert(!error);
	pg_brick_link(antispoof, col_east, &error);
	g_assert(!error);

	/* enable ARP antispoof with a wrong IP */
	pg_antispoof_arp_enable(antispoof);
	g_assert(!pg_antispoof_arp_add(antispoof, inside_ip, &error));
	g_assert(!error);

	/* replay traffic */
	for (i = 0; i < pkts_nb; i++) {
		packet = build_packet(pkts[i], pkts_size[i]);
		pg_brick_poll(gen_west, &packet_count, &error);
		g_assert(!error);
		g_assert(packet_count == 1);
		filtered_pkts = pg_brick_west_burst_get(col_east,
							&filtered_pkts_mask,
							&error);
		g_assert(!error);
		g_assert(pg_mask_count(filtered_pkts_mask) == 0);
		pg_packets_free(filtered_pkts, filtered_pkts_mask);
		rte_pktmbuf_free(packet);
	}

	/* disable ARP antispoof, should now pass */
	pg_antispoof_arp_disable(antispoof);

	/* replay traffic */
	for (i = 0; i < pkts_nb; i++) {
		packet = build_packet(pkts[i], pkts_size[i]);
		pg_brick_poll(gen_west, &packet_count, &error);
		g_assert(!error);
		g_assert(packet_count == 1);
		filtered_pkts = pg_brick_west_burst_get(col_east,
							&filtered_pkts_mask,
							&error);
		g_assert(!error);
		g_assert(pg_mask_count(filtered_pkts_mask) == 1);
		pg_packets_free(filtered_pkts, filtered_pkts_mask);
		rte_pktmbuf_free(packet);
	}

	pg_brick_destroy(gen_west);
	pg_brick_destroy(antispoof);
	pg_brick_destroy(col_east);
}

static void test_antispoof_arp_request(void)
{
#	include "test-arp-request.c"
	const unsigned char *pkts[] = {pkt1};
	int pkts_size[] = {42};
	uint16_t pkts_nb = 1;
	struct ether_addr inside_mac;
	uint32_t inside_ip;

	pg_scan_ether_addr(&inside_mac, "00:e0:81:d5:02:91");
	inside_ip = htobe32(IPv4(192, 168, 21, 253));
	test_antispoof_generic(pkts, pkts_size, pkts_nb, inside_mac, inside_ip);
}

static void test_antispoof_arp_response(void)
{
#	include "test-arp-response.c"
	const unsigned char *pkts[] = {pkt1};
	int pkts_size[] = {42};
	uint16_t pkts_nb = 1;
	struct ether_addr inside_mac;
	uint32_t inside_ip;

	pg_scan_ether_addr(&inside_mac, "00:18:b9:56:2e:73");
	inside_ip = htobe32(IPv4(192, 168, 21, 2));
	test_antispoof_generic(pkts, pkts_size, pkts_nb, inside_mac, inside_ip);
}

static void test_antispoof_arp_gratuitous(void)
{
#	include "test-arp-gratuitous.c"
	const unsigned char *pkts[] = {pkt1};
	int pkts_size[] = {42};
	uint16_t pkts_nb = 1;
	struct ether_addr inside_mac;
	uint32_t inside_ip;

	pg_scan_ether_addr(&inside_mac, "00:23:df:ff:c9:23");
	inside_ip = htobe32(IPv4(192, 168, 22, 56));
	test_antispoof_generic(pkts, pkts_size, pkts_nb, inside_mac, inside_ip);
}

static void test_antispoof_empty_burst(void)
{
	struct pg_brick *antispoof;
	struct pg_brick *fail;
	struct ether_addr inside_mac;
	struct rte_mbuf **pkts;
	struct pg_error *error = NULL;
	struct ether_addr eth;

	pg_scan_ether_addr(&eth, "00:18:b9:56:2e:73");
	pg_scan_ether_addr(&inside_mac, "00:26:df:ff:c9:44");

	antispoof = pg_antispoof_new("antispoof", PG_EAST_SIDE,
				     &inside_mac, &error);
	g_assert(!error);

	fail = pg_fail_new("fail", &error);
	g_assert(!error);

	pg_brick_link(antispoof, fail, &error);
	g_assert(!error);

	pkts = pg_packets_append_ether(pg_packets_create(pg_mask_firsts(64)),
				       pg_mask_firsts(64), &eth, &eth,
				       ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, pg_mask_firsts(32), 1, 2, 0, 0);
	pg_brick_burst_to_east(antispoof, 0, pkts, pg_mask_firsts(64), &error);
	g_assert(!error);

	pg_brick_destroy(antispoof);
	pg_brick_destroy(fail);
	pg_packets_free(pkts, pg_mask_firsts(64));
	g_free(pkts);
}

static int test_antispoof_filter(struct pg_brick *antispoof,
				 struct rte_mbuf *packet)
{
	struct pg_brick *gen_west;
	struct pg_brick *col_east;
	struct pg_error *error = NULL;
	uint16_t packet_count;
	uint64_t filtered_pkts_mask;

	/* [generator>]--[antispoof]--[collector] */
	gen_west = pg_packetsgen_new("gen_west", 1, 1, PG_EAST_SIDE,
				     &packet, 1, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	pg_brick_chained_links(&error, gen_west, antispoof, col_east);
	g_assert(!error);

	/* replay traffic */
	pg_brick_poll(gen_west, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == 1);
	pg_brick_west_burst_get(col_east, &filtered_pkts_mask, &error);
	pg_brick_unlink(antispoof, &error);
	g_assert(!error);
	pg_brick_destroy(gen_west);
	pg_brick_destroy(col_east);

	return filtered_pkts_mask;
}

static void test_antispoof_ndp(void)
{
#	include "test-ndp.c"
	struct ether_addr inside_mac;
	uint8_t ip[16];
	struct pg_brick *antispoof;
	struct pg_error *error = NULL;
	struct rte_mbuf *packet;

	pg_scan_ether_addr(&inside_mac, "52:54:00:12:34:02");
	antispoof = pg_antispoof_new("antispoof", PG_EAST_SIDE,
				     &inside_mac, &error);
	g_assert(!error);

	/* enable ndp antispoof */
	pg_antispoof_ndp_enable(antispoof);
	pg_ip_from_str(ip, "2001:db8:2000:aff0::2");
	pg_antispoof_ndp_add(antispoof, ip, &error);
	g_assert(!error);

	/* legit packet */
	packet = build_packet(pkt0, 86);
	g_assert(test_antispoof_filter(antispoof, packet) > 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad target address */
	packet = build_packet(pkt1, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad link layer address */
	packet = build_packet(pkt2, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));

	/* disable ndp antispoof */
	pg_antispoof_ndp_disable(antispoof);

	/* legit packet */
	packet = build_packet(pkt0, 86);
	g_assert(test_antispoof_filter(antispoof, packet) > 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad target address */
	packet = build_packet(pkt1, 86);
	g_assert(test_antispoof_filter(antispoof, packet) > 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad link layer address */
	packet = build_packet(pkt2, 86);
	g_assert(test_antispoof_filter(antispoof, packet) > 0);
	pg_packets_free(&packet, pg_mask_firsts(1));

	/* enable ndp antispoof and remove all addresses */
	pg_antispoof_ndp_enable(antispoof);
	pg_antispoof_ndp_del_all(antispoof);

	/* legit packet */
	packet = build_packet(pkt0, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad target address */
	packet = build_packet(pkt1, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad link layer address */
	packet = build_packet(pkt2, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));

	/* add several bad addresses */
	pg_ip_from_str(ip, "2001:db8:2000:aff0::42");
	pg_antispoof_ndp_add(antispoof, ip, &error);
	g_assert(!error);
	pg_ip_from_str(ip, "2001:db8:2000:aff0::43");
	pg_antispoof_ndp_add(antispoof, ip, &error);
	g_assert(!error);

	/* legit packet */
	packet = build_packet(pkt0, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad target address */
	packet = build_packet(pkt1, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad link layer address */
	packet = build_packet(pkt2, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));

	/* add correct address */
	pg_ip_from_str(ip, "2001:db8:2000:aff0::2");
	pg_antispoof_ndp_add(antispoof, ip, &error);
	g_assert(!error);

	/* legit packet */
	packet = build_packet(pkt0, 86);
	g_assert(test_antispoof_filter(antispoof, packet) > 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad target address */
	packet = build_packet(pkt1, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));
	/* bad link layer address */
	packet = build_packet(pkt2, 86);
	g_assert(test_antispoof_filter(antispoof, packet) == 0);
	pg_packets_free(&packet, pg_mask_firsts(1));

	pg_brick_destroy(antispoof);
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

	pg_test_add_func("/antispoof/mac",
			test_antispoof_mac);
	pg_test_add_func("/antispoof/rarp",
			test_antispoof_rarp);
	pg_test_add_func("/antispoof/arp/request",
			test_antispoof_arp_request);
	pg_test_add_func("/antispoof/arp/response",
			test_antispoof_arp_response);
	pg_test_add_func("/antispoof/arp/gratuitous",
			test_antispoof_arp_gratuitous);
	pg_test_add_func("/antispoof/arp/disable",
			test_pg_antispoof_arp_disable);
	pg_test_add_func("/antispoof/mac/burst_not_propagate",
			test_antispoof_empty_burst);
	pg_test_add_func("/antispoof/ndp",
			test_antispoof_ndp);
	r = g_test_run();
	pg_stop();
	return r;
}
