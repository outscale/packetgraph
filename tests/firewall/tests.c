/* Copyreght 2015 Outscale SAS
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

#define	__FAVOR_BSD
#include <arpa/inet.h>
/* #include <net/ethernet.h> */
/* #include <netinet/ether.h> */
#include <netinet/ip.h>
/* #include <netinet/udp.h> */

#include <rte_config.h>
#include <rte_ether.h>
#include <pcap/pcap.h>

#include <packetgraph/packetgraph.h>
#include <packetgraph/firewall.h>
#include "brick-int.h"
#include "collect.h"
#include "packetsgen.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/mac.h"
#include "utils/bitmask.h"

static struct rte_mbuf *build_ip_packet(const char *src_ip,
					const char *dst_ip, uint16_t data)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mp);
	uint16_t len = sizeof(struct ether_hdr) + sizeof(struct ip) +
		sizeof(uint16_t);
	struct ether_hdr *eth;
	struct ip *ip;
	uint16_t *payload_ip;

	pkt->pkt_len = len;
	pkt->data_len = len;
	pkt->nb_segs = 1;
	pkt->next = NULL;

	/* ethernet header */
	eth = rte_pktmbuf_mtod(pkt, struct ether_hdr*);
	memset(eth, 0, len);
	eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);

	/* ipv4 header */
	ip = (struct ip *)(eth + 1);
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_off = 0;
	ip->ip_ttl = 64;

	/* FEAR ! this is CHAOS ! */
	ip->ip_p = 16;
	ip->ip_len = htons(sizeof(struct ip) + 1);
	inet_pton(AF_INET, src_ip, &ip->ip_src.s_addr);
	inet_pton(AF_INET, dst_ip, &ip->ip_dst.s_addr);

	/* write some data */
	payload_ip = (uint16_t *)(ip + 1);
	*payload_ip = data;
	return pkt;
}

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

static struct rte_mbuf *build_non_ip_packet(void)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mp);
	uint8_t *payload;
	struct ether_hdr *eth;
	uint16_t len = sizeof(struct ether_hdr) + 1;

	pkt->pkt_len = len;
	pkt->data_len = len;
	pkt->nb_segs = 1;
	pkt->next = NULL;

	/* ethernet header */
	eth = rte_pktmbuf_mtod(pkt, struct ether_hdr*);
	memset(eth, 0, len);
	eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_ARP);

	/* write some data */
	payload = (uint8_t *)(eth + 1);
	*payload = 42;
	return pkt;
}

static void firewall_filter_rules(enum pg_side dir)
{
	struct pg_brick *gen;
	struct pg_brick *fw;
	struct pg_brick *col;
	struct pg_error *error = NULL;
	uint16_t i;
	static uint16_t nb = 30;
	struct rte_mbuf *packets[nb];
	uint64_t filtered_pkts_mask;
	struct rte_mbuf **filtered_pkts;
	uint64_t bit;
	uint16_t packet_count;
	struct ip *ip;
	struct ether_hdr *eth;

	/* create and connect 3 bricks: generator -> firewall -> collector */
	gen = pg_packetsgen_new("gen", 2, 2, pg_flip_side(dir), packets, nb, &error);
	g_assert(!error);
	fw = pg_firewall_new("fw", 2, 2, PG_NONE, &error);
	g_assert(!error);
	col = pg_collect_new("col", 2, 2, &error);
	g_assert(!error);
	/* revert link if needed */
	if (dir == WEST_SIDE) {
		pg_brick_link(gen, fw, &error);
		g_assert(!error);
		pg_brick_link(fw, col, &error);
		g_assert(!error);
	} else {
		pg_brick_link(col, fw, &error);
		g_assert(!error);
		pg_brick_link(fw, gen, &error);
		g_assert(!error);
	}

	/* build some UDP packets mixed sources */
	for (i = 0; i < nb; i++)
		switch (i % 3) {
		case 0:
			packets[i] = build_ip_packet("10.0.0.1",
						     "10.0.0.255", i);
			break;
		case 1:
			packets[i] = build_ip_packet("10.0.0.2",
						     "10.0.0.255", i);
			break;
		case 2:
			packets[i] = build_ip_packet("10.0.0.3",
						     "10.0.0.255", i);
			break;
		}

	/* configure firewall to allow traffic from 10.0.0.1 */
	g_assert(!pg_firewall_rule_add(fw, "src host 10.0.0.1", dir, 0,
				       &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let's burst ! */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col,
							&filtered_pkts_mask,
							&error);
	else
		filtered_pkts = pg_brick_east_burst_get(col,
							&filtered_pkts_mask,
							&error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == nb / 3);
	for (; filtered_pkts_mask;) {
		uint32_t tmp;

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);

		inet_pton(AF_INET, "10.0.0.1", &tmp);
		g_assert(ip->ip_src.s_addr == tmp);
	}

	/* now allow packets from 10.0.0.2 */
	g_assert(!pg_firewall_rule_add(fw, "src host 10.0.0.2", dir, 0,
				       &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let it goooo */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = pg_brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == nb * 2 / 3);
	for (; filtered_pkts_mask;) {
		uint32_t tmp1;
		uint32_t tmp2;

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0 || i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);
		inet_pton(AF_INET, "10.0.0.1", &tmp1);
		inet_pton(AF_INET, "10.0.0.2", &tmp2);
		g_assert(ip->ip_src.s_addr == tmp1 ||
			 ip->ip_src.s_addr == tmp2);
	}

	/* test that flush really blocks */
	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);
	g_assert(pg_brick_reset(col, &error) >= 0);
	g_assert(!error);

	/* let it goooo */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = pg_brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == 0);

	/* flush and only allow packets from 10.0.0.2 */
	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_rule_add(fw, "src host 10.0.0.2",
				       dir, 0, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let it goooo */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = pg_brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == nb / 3);
	for (; filtered_pkts_mask;) {
		uint32_t tmp;

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);
		inet_pton(AF_INET, "10.0.0.2", &tmp);
		g_assert(ip->ip_src.s_addr == tmp);
	}

	/* flush and make two rules in one */
	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_rule_add(fw, "src host (10.0.0.1 or 10.0.0.2)",
				       dir, 0, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let it goooo */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = pg_brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == nb * 2 / 3);
	for (; filtered_pkts_mask;) {
		uint32_t tmp1;		
		uint32_t tmp2;

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0 || i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);
		inet_pton(AF_INET, "10.0.0.1", &tmp1);
		inet_pton(AF_INET, "10.0.0.2", &tmp2);
		g_assert(ip->ip_src.s_addr == tmp1 ||
			 ip->ip_src.s_addr == tmp2);
	}

	/* flush and revert rules, packets should not pass */
	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_rule_add(fw, "src host (10.0.0.1)",
				       pg_flip_side(dir), 0, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);
	g_assert(pg_brick_reset(col, &error) >= 0);
	g_assert(!error);

	/* let it goooo */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = pg_brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == 0);

	/* flush and allow packets from both sides */
	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_rule_add(fw, "src host (10.0.0.1)",
				       MAX_SIDE, 0, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let it goooo */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	if (dir == WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = pg_brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == nb / 3);
	for (; filtered_pkts_mask;) {
		uint32_t tmp;

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);
		inet_pton(AF_INET, "10.0.0.1", &tmp);
		g_assert(ip->ip_src.s_addr == tmp);
	}

	/* inverse generator and collector to test both sides */
	pg_brick_unlink(fw, &error);
	g_assert(!error);
	if (dir == WEST_SIDE) {
		pg_brick_link(col, fw, &error);
		g_assert(!error);
		pg_brick_link(fw, gen, &error);
		g_assert(!error);
	} else {
		pg_brick_link(gen, fw, &error);
		g_assert(!error);
		pg_brick_link(fw, col, &error);
		g_assert(!error);
	}

	/* let it goooo */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	if (dir == WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = pg_brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == nb / 3);
	for (; filtered_pkts_mask;) {
		uint32_t tmp;

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);
		inet_pton(AF_INET, "10.0.0.1", &tmp);
		g_assert(ip->ip_src.s_addr == tmp);
	}

	/* clean */
	for (i = 0; i < nb; i++)
		rte_pktmbuf_free(packets[i]);
	pg_brick_destroy(gen);
	pg_brick_destroy(fw);
	pg_brick_destroy(col);
}

static void firewall_replay(const unsigned char *pkts[],
			    int pkts_nb, int *pkts_size)
{
	struct pg_brick *gen_west, *gen_east;
	struct pg_brick *fw;
	struct pg_brick *col_west, *col_east;
	struct pg_error *error = NULL;
	uint16_t i, packet_count;
	struct rte_mbuf *packet;
	struct ether_hdr *eth;
	uint64_t filtered_pkts_mask;
	struct rte_mbuf **filtered_pkts;
	struct ether_addr tmp_addr;

	/* have some collectors and generators on each sides
	 * [collector]--[generator>]--[firewall]--[<generator]--[collector]
	 * 10.0.2.15                                         173.194.40.111
	 * 8:0:27:b6:5:16                                   52:54:0:12:35:2
	 */
	gen_west = pg_packetsgen_new("gen_west", 1, 1, EAST_SIDE, &packet, 1,
				  &error);
	g_assert(!error);
	gen_east = pg_packetsgen_new("gen_east", 1, 1, WEST_SIDE, &packet, 1,
				  &error);
	g_assert(!error);
	fw = pg_firewall_new("fw", 1, 1, PG_NONE, &error);
	g_assert(!error);
	col_west = pg_collect_new("col_west", 1, 1, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", 1, 1, &error);
	g_assert(!error);
	pg_brick_link(col_west, gen_west, &error);
	g_assert(!error);
	pg_brick_link(gen_west, fw, &error);
	g_assert(!error);
	pg_brick_link(fw, gen_east, &error);
	g_assert(!error);
	pg_brick_link(gen_east, col_east, &error);
	g_assert(!error);

	/* open all traffic of 10.0.2.15 from the west side of the firewall
	 * returning traffic should be allowed due to STATEFUL option
	 */
	g_assert(!pg_firewall_rule_add(fw, "src host 10.0.2.15",
				       WEST_SIDE, 1, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* replay traffic */
	for (i = 0; i < pkts_nb; i++) {
		struct ip *ip;
		uint32_t tmp1;
		uint32_t tmp2;		

		packet = build_packet(pkts[i], pkts_size[i]);
		eth = rte_pktmbuf_mtod(packet, struct ether_hdr*);
		ip = (struct ip *)(eth + 1);
		inet_pton(AF_INET, "10.0.2.15", &tmp1);
		inet_pton(AF_INET, "173.194.40.111", &tmp2);

		if (ip->ip_src.s_addr == tmp1) {
			uint32_t tmp;

			pg_brick_poll(gen_west, &packet_count, &error);
			g_assert(!error);
			g_assert(packet_count == 1);
			filtered_pkts = pg_brick_west_burst_get(col_east,
				&filtered_pkts_mask, &error);
			g_assert(!error);
			g_assert(pg_mask_count(filtered_pkts_mask) == 1);
			/* check eth source address */
			eth = rte_pktmbuf_mtod(filtered_pkts[0],
					       struct ether_hdr*);
			pg_scan_ether_addr(&tmp_addr, "08:00:27:b6:05:16");
			g_assert(is_same_ether_addr(&eth->s_addr, &tmp_addr));
			/* check ip source address */
			ip = (struct ip *)(eth + 1);
			inet_pton(AF_INET, "10.0.2.15", &tmp);
			g_assert(ip->ip_src.s_addr == tmp);
		} else if (ip->ip_src.s_addr == tmp2) {
			uint32_t tmp;

			pg_brick_poll(gen_east, &packet_count, &error);
			g_assert(!error);
			g_assert(packet_count == 1);
			filtered_pkts = pg_brick_east_burst_get(col_west,
				&filtered_pkts_mask, &error);
			g_assert(!error);
			g_assert(pg_mask_count(filtered_pkts_mask) == 1);
			/* check eth source address */
			eth = rte_pktmbuf_mtod(filtered_pkts[0],
					       struct ether_hdr*);
			pg_scan_ether_addr(&tmp_addr, "52:54:00:12:35:02");
			g_assert(is_same_ether_addr(&eth->s_addr, &tmp_addr));
			/* check ip source address */
			ip = (struct ip *)(eth + 1);
			inet_pton(AF_INET, "173.194.40.111", &tmp);
			g_assert(ip->ip_src.s_addr == tmp);
		} else
			g_assert(0);

		rte_pktmbuf_free(packet);
		/* ensure that connexion is tracked even when reloading */
		g_assert(!pg_firewall_rule_add(fw, "src host 6.6.6.6",
					       WEST_SIDE, 0, &error));
		g_assert(!error);
		g_assert(!pg_firewall_reload(fw, &error));
		g_assert(!error);
	}

	/* clean */
	pg_brick_destroy(gen_west);
	pg_brick_destroy(gen_east);
	pg_brick_destroy(col_west);
	pg_brick_destroy(col_east);
	pg_brick_destroy(fw);
}

static void firewall_noip(enum pg_side dir)
{
	struct pg_brick *gen;
	struct pg_brick *fw;
	struct pg_brick *col;
	struct pg_error *error = NULL;
	uint16_t i;
	static uint16_t nb = 30;
	struct rte_mbuf *packets[nb];
	uint64_t filtered_pkts_mask;
	uint16_t packet_count;

	/* create and connect 3 bricks: generator -> firewall -> collector */
	gen = pg_packetsgen_new("gen", 2, 2, pg_flip_side(dir), packets, nb, &error);
	g_assert(!error);
	fw = pg_firewall_new("fw", 2, 2, PG_NONE, &error);
	g_assert(!error);
	col = pg_collect_new("col", 2, 2, &error);
	g_assert(!error);
	/* revert link if needed */
	if (dir == WEST_SIDE) {
		pg_brick_link(gen, fw, &error);
		g_assert(!error);
		pg_brick_link(fw, col, &error);
		g_assert(!error);
	} else {
		pg_brick_link(col, fw, &error);
		g_assert(!error);
		pg_brick_link(fw, gen, &error);
		g_assert(!error);
	}

	/* build some non-IP packets */
	for (i = 0; i < nb; i++)
		packets[i] = build_non_ip_packet();

	/* let's burst ! */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		pg_brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		pg_brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(pg_mask_count(filtered_pkts_mask) == nb);

	/* clean */
	for (i = 0; i < nb; i++)
		rte_pktmbuf_free(packets[i]);
	pg_brick_destroy(gen);
	pg_brick_destroy(fw);
	pg_brick_destroy(col);
}

static void test_firewall_filter(void)
{
	firewall_filter_rules(WEST_SIDE);
	firewall_filter_rules(EAST_SIDE);
}

static void test_firewall_tcp(void)
{
#	include "test-tcp.c"
	const unsigned char *pkts[] = {pkt1, pkt2, pkt3, pkt4, pkt5,
		pkt6, pkt7, pkt8, pkt9, pkt10};
	int pkts_size[] = {74, 60, 54, 58, 60, 575, 54, 60, 54, 60};

	firewall_replay(pkts, 10, pkts_size);
}

static void test_firewall_icmp(void)
{
#	include "test-icmp.c"
	const unsigned char *pkts[] = {pkt1, pkt2, pkt3, pkt4, pkt5,
		pkt6, pkt7, pkt8, pkt9, pkt10};
	int pkts_size[] = {98, 98, 98, 98, 98, 98, 98, 98, 98, 98};

	firewall_replay(pkts, 10, pkts_size);
}

static void test_firewall_noip(void)
{
	firewall_noip(WEST_SIDE);
	firewall_noip(EAST_SIDE);
}

static void test_firewall(void)
{
	g_test_add_func("/firewall/filter", test_firewall_filter);
	g_test_add_func("/firewall/tcp", test_firewall_tcp);
	g_test_add_func("/firewall/icmp", test_firewall_icmp);
	g_test_add_func("/firewall/noip", test_firewall_noip);
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

	test_firewall();
	r = g_test_run();

	pg_stop();
	return r;
}
