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
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pcap/pcap.h>

#include <packetgraph/packetgraph.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/brick.h>
#include <packetgraph/firewall.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/collect.h>
#include <packetgraph/packetsgen.h>
#include <packetgraph/packets.h>

static struct rte_mbuf *build_ip_packet(const char *src_ip,
					const char *dst_ip, uint16_t data)
{
	struct rte_mempool *mp = get_mempool();
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mp);
	uint16_t len = sizeof(struct ether_header) + sizeof(struct ip) +
		sizeof(uint16_t);
	struct ether_header *eth;
	struct ip *ip;
	uint16_t *payload_ip;

	pkt->pkt_len = len;
	pkt->data_len = len;
	pkt->nb_segs = 1;
	pkt->next = NULL;

	/* ethernet header */
	eth = rte_pktmbuf_mtod(pkt, struct ether_header*);
	memset(eth, 0, len);
	eth->ether_type = htobe16(ETHERTYPE_IP);

	/* ipv4 header */
	ip = (struct ip *)(eth + 1);
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_off = 0;
	ip->ip_ttl = 64;

	/* FEAR ! this is CHAOS ! */
	ip->ip_p = 16;
	ip->ip_len = htons(sizeof(struct ip) + 1);
	ip->ip_src.s_addr = inet_addr(src_ip);
	ip->ip_dst.s_addr = inet_addr(dst_ip);

	/* write some data */
	payload_ip = (uint16_t *)(ip + 1);
	*payload_ip = data;
	return pkt;
}

static struct rte_mbuf *build_packet(const unsigned char *data, size_t len)
{
	struct rte_mempool *mp = get_mempool();
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
	struct rte_mempool *mp = get_mempool();
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mp);
	uint8_t *payload;
	struct ether_header *eth;
	uint16_t len = sizeof(struct ether_header) + 1;

	pkt->pkt_len = len;
	pkt->data_len = len;
	pkt->nb_segs = 1;
	pkt->next = NULL;

	/* ethernet header */
	eth = rte_pktmbuf_mtod(pkt, struct ether_header*);
	memset(eth, 0, len);
	eth->ether_type = htobe16(ETHERTYPE_ARP);

	/* write some data */
	payload = (uint8_t *)(eth + 1);
	*payload = 42;
	return pkt;
}

static void firewall_filter_rules(enum side dir)
{
	struct brick *gen;
	struct brick *fw;
	struct brick *col;
	struct switch_error *error = NULL;
	uint16_t i;
	int ret;
	static uint16_t nb = 30;
	struct rte_mbuf *packets[nb];
	uint64_t filtered_pkts_mask;
	struct rte_mbuf **filtered_pkts;
	uint64_t bit;
	uint16_t packet_count;
	struct ip *ip;
	struct ether_header *eth;

	/* create and connect 3 bricks: generator -> firewall -> collector */
	gen = packetsgen_new("gen", 2, 2, flip_side(dir), packets, nb, &error);
	g_assert(!error);
	fw = firewall_new("fw", 2, 2, &error);
	g_assert(!error);
	col = collect_new("col", 2, 2, &error);
	g_assert(!error);
	/* revert link if needed */
	if (dir == WEST_SIDE) {
		brick_link(gen, fw, &error);
		g_assert(!error);
		brick_link(fw, col, &error);
		g_assert(!error);
	} else {
		brick_link(col, fw, &error);
		g_assert(!error);
		brick_link(fw, gen, &error);
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
	ret = firewall_rule_add(fw, "src host 10.0.0.1", dir, 0, &error);
	g_assert(!error);
	g_assert(ret == 0);
	ret = firewall_reload(fw, &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* let's burst ! */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	printf("mask_count(filtered_pkts_mask): %i\n",
	       mask_count(filtered_pkts_mask));
	g_assert(mask_count(filtered_pkts_mask) == nb / 3);
	for (; filtered_pkts_mask;) {
		low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_header*);
		ip = (struct ip *)(eth + 1);
		g_assert(ip->ip_src.s_addr == inet_addr("10.0.0.1"));
	}
	packets_free(filtered_pkts, mask_firsts(nb));

	/* now allow packets from 10.0.0.2 */
	ret = firewall_rule_add(fw, "src host 10.0.0.2", dir, 0, &error);
	g_assert(!error);
	g_assert(ret == 0);
	ret = firewall_reload(fw, &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* let it goooo */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(mask_count(filtered_pkts_mask) == nb * 2 / 3);
	for (; filtered_pkts_mask;) {
		low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0 || i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_header*);
		ip = (struct ip *)(eth + 1);
		g_assert(ip->ip_src.s_addr == inet_addr("10.0.0.1") ||
			 ip->ip_src.s_addr == inet_addr("10.0.0.2"));
	}
	packets_free(filtered_pkts, mask_firsts(nb));

	/* test that flush really blocks */
	firewall_rule_flush(fw);
	ret = firewall_reload(fw, &error);
	g_assert(!error);
	g_assert(ret == 0);

	/* let it goooo */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(mask_count(filtered_pkts_mask) == 0);

	/* flush and only allow packets from 10.0.0.2 */
	firewall_rule_flush(fw);
	ret = firewall_rule_add(fw, "src host 10.0.0.2", dir, 0, &error);
	g_assert(!error);
	g_assert(ret == 0);
	ret = firewall_reload(fw, &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* let it goooo */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(mask_count(filtered_pkts_mask) == nb / 3);
	for (; filtered_pkts_mask;) {
		low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_header*);
		ip = (struct ip *)(eth + 1);
		g_assert(ip->ip_src.s_addr == inet_addr("10.0.0.2"));
	}
	packets_free(filtered_pkts, mask_firsts(nb));

	/* flush and make two rules in one */
	firewall_rule_flush(fw);
	ret = firewall_rule_add(fw, "src host (10.0.0.1 or 10.0.0.2)", dir, 0,
				&error);
	g_assert(!error);
	g_assert(ret == 0);
	ret = firewall_reload(fw, &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* let it goooo */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(mask_count(filtered_pkts_mask) == nb * 2 / 3);
	for (; filtered_pkts_mask;) {
		low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0 || i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_header*);
		ip = (struct ip *)(eth + 1);
		g_assert(ip->ip_src.s_addr == inet_addr("10.0.0.1") ||
			 ip->ip_src.s_addr == inet_addr("10.0.0.2"));
	}
	packets_free(filtered_pkts, mask_firsts(nb));

	/* flush and revert rules, packets should not pass */
	firewall_rule_flush(fw);
	ret = firewall_rule_add(fw, "src host (10.0.0.1)", flip_side(dir), 0,
				&error);
	g_assert(!error);
	g_assert(ret == 0);
	ret = firewall_reload(fw, &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* let it goooo */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(mask_count(filtered_pkts_mask) == 0);

	/* flush and allow packets from both sides */
	firewall_rule_flush(fw);
	ret = firewall_rule_add(fw, "src host (10.0.0.1)", MAX_SIDE, 0, &error);
	g_assert(!error);
	g_assert(ret == 0);
	ret = firewall_reload(fw, &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* let it goooo */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(mask_count(filtered_pkts_mask) == nb / 3);
	for (; filtered_pkts_mask;) {
		low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_header*);
		ip = (struct ip *)(eth + 1);
		g_assert(ip->ip_src.s_addr == inet_addr("10.0.0.1"));
	}
	packets_free(filtered_pkts, mask_firsts(nb));

	/* inverse generator and collector to test both sides */
	brick_unlink(fw, &error);
	g_assert(!error);
	if (dir == WEST_SIDE) {
		brick_link(col, fw, &error);
		g_assert(!error);
		brick_link(fw, gen, &error);
		g_assert(!error);
	} else {
		brick_link(gen, fw, &error);
		g_assert(!error);
		brick_link(fw, col, &error);
		g_assert(!error);
	}

	/* let it goooo */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(mask_count(filtered_pkts_mask) == nb / 3);
	for (; filtered_pkts_mask;) {
		low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_header*);
		ip = (struct ip *)(eth + 1);
		g_assert(ip->ip_src.s_addr == inet_addr("10.0.0.1"));
	}
	packets_free(filtered_pkts, mask_firsts(nb));

	/* clean */
	for (i = 0; i < nb; i++)
		rte_pktmbuf_free(packets[i]);
	brick_destroy(gen);
	brick_destroy(fw);
	brick_destroy(col);
}

static void firewall_replay(const unsigned char *pkts[],
			    int pkts_nb, int *pkts_size)
{
	struct brick *gen_west, *gen_east;
	struct brick *fw;
	struct brick *col_west, *col_east;
	struct switch_error *error = NULL;
	uint16_t i, packet_count;
	struct rte_mbuf *packet;
	struct ether_header *eth;
	struct ip *ip;
	uint64_t filtered_pkts_mask;
	struct rte_mbuf **filtered_pkts;
	int ret;

	/* have some collectors and generators on each sides
	 * [collector]--[generator>]--[firewall]--[<generator]--[collector]
	 * 10.0.2.15                                         173.194.40.111
	 * 8:0:27:b6:5:16                                   52:54:0:12:35:2
	 */
	gen_west = packetsgen_new("gen_west", 1, 1, EAST_SIDE, &packet, 1,
				  &error);
	g_assert(!error);
	gen_east = packetsgen_new("gen_east", 1, 1, WEST_SIDE, &packet, 1,
				  &error);
	g_assert(!error);
	fw = firewall_new("fw", 1, 1, &error);
	g_assert(!error);
	col_west = collect_new("col_west", 1, 1, &error);
	g_assert(!error);
	col_east = collect_new("col_east", 1, 1, &error);
	g_assert(!error);
	brick_link(col_west, gen_west, &error);
	g_assert(!error);
	brick_link(gen_west, fw, &error);
	g_assert(!error);
	brick_link(fw, gen_east, &error);
	g_assert(!error);
	brick_link(gen_east, col_east, &error);
	g_assert(!error);

	/* open all traffic of 10.0.2.15 from the west side of the firewall
	 * returning traffic should be allowed due to STATEFUL option
	 */
	ret = firewall_rule_add(fw, "src host 10.0.2.15", WEST_SIDE, 1, &error);
	g_assert(!error);
	g_assert(ret == 0);
	ret = firewall_reload(fw, &error);
	g_assert(!error);
	g_assert(ret == 0);

	/* replay traffic */
	for (i = 0; i < pkts_nb; i++) {
		packet = build_packet(pkts[i], pkts_size[i]);
		eth = rte_pktmbuf_mtod(packet, struct ether_header*);
		ip = (struct ip *)(eth + 1);

		if (ip->ip_src.s_addr == inet_addr("10.0.2.15")) {
			brick_poll(gen_west, &packet_count, &error);
			g_assert(!error);
			g_assert(packet_count == 1);
			filtered_pkts = brick_west_burst_get(col_east,
				&filtered_pkts_mask, &error);
			g_assert(!error);
			g_assert(mask_count(filtered_pkts_mask) == 1);
			/* check eth source address */
			eth = rte_pktmbuf_mtod(filtered_pkts[0],
					       struct ether_header*);
			g_assert(strcmp(ether_ntoa((struct ether_addr *)
				    eth->ether_shost), "8:0:27:b6:5:16") == 0);
			/* check ip source address */
			ip = (struct ip *)(eth + 1);
			g_assert(ip->ip_src.s_addr == inet_addr("10.0.2.15"));
		} else if (ip->ip_src.s_addr == inet_addr("173.194.40.111")) {
			brick_poll(gen_east, &packet_count, &error);
			g_assert(!error);
			g_assert(packet_count == 1);
			filtered_pkts = brick_east_burst_get(col_west,
				&filtered_pkts_mask, &error);
			g_assert(!error);
			g_assert(mask_count(filtered_pkts_mask) == 1);
			/* check eth source address */
			eth = rte_pktmbuf_mtod(filtered_pkts[0],
					       struct ether_header*);
			g_assert(strcmp(ether_ntoa((struct ether_addr *)
				    eth->ether_shost), "52:54:0:12:35:2") == 0);
			/* check ip source address */
			ip = (struct ip *)(eth + 1);
			g_assert(ip->ip_src.s_addr ==
				 inet_addr("173.194.40.111"));
		} else
			g_assert(0);

		packets_free(filtered_pkts, mask_firsts(1));
		rte_pktmbuf_free(packet);
		/* ensure that connexion is tracked even when reloading */
		ret = firewall_rule_add(fw, "src host 6.6.6.6", WEST_SIDE, 0,
					&error);
		g_assert(!error);
		g_assert(ret == 0);
		ret = firewall_reload(fw, &error);
		g_assert(!error);
		g_assert(ret == 0);
	}

	/* clean */
	brick_destroy(gen_west);
	brick_destroy(gen_east);
	brick_destroy(col_west);
	brick_destroy(col_east);
	brick_destroy(fw);
}

static void firewall_noip(enum side dir)
{
	struct brick *gen;
	struct brick *fw;
	struct brick *col;
	struct switch_error *error = NULL;
	uint16_t i;
	static uint16_t nb = 30;
	struct rte_mbuf *packets[nb];
	uint64_t filtered_pkts_mask;
	struct rte_mbuf **filtered_pkts;
	uint16_t packet_count;

	/* create and connect 3 bricks: generator -> firewall -> collector */
	gen = packetsgen_new("gen", 2, 2, flip_side(dir), packets, nb, &error);
	g_assert(!error);
	fw = firewall_new("fw", 2, 2, &error);
	g_assert(!error);
	col = collect_new("col", 2, 2, &error);
	g_assert(!error);
	/* revert link if needed */
	if (dir == WEST_SIDE) {
		brick_link(gen, fw, &error);
		g_assert(!error);
		brick_link(fw, col, &error);
		g_assert(!error);
	} else {
		brick_link(col, fw, &error);
		g_assert(!error);
		brick_link(fw, gen, &error);
		g_assert(!error);
	}

	/* build some non-IP packets */
	for (i = 0; i < nb; i++)
		packets[i] = build_non_ip_packet();

	/* let's burst ! */
	brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == WEST_SIDE)
		filtered_pkts = brick_west_burst_get(col, &filtered_pkts_mask,
						     &error);
	else
		filtered_pkts = brick_east_burst_get(col, &filtered_pkts_mask,
						     &error);
	g_assert(!error);
	g_assert(mask_count(filtered_pkts_mask) == nb);
	packets_free(filtered_pkts, mask_firsts(nb));

	/* clean */
	for (i = 0; i < nb; i++)
		rte_pktmbuf_free(packets[i]);
	brick_destroy(gen);
	brick_destroy(fw);
	brick_destroy(col);
}

static void test_firewall_filter(void)
{
	firewall_filter_rules(WEST_SIDE);
	firewall_filter_rules(EAST_SIDE);
}

static void test_firewall_tcp(void)
{
#	include "firewall-brick-tcp.c"
	const unsigned char *pkts[] = {pkt1, pkt2, pkt3, pkt4, pkt5,
		pkt6, pkt7, pkt8, pkt9, pkt10};
	int pkts_size[] = {74, 60, 54, 58, 60, 575, 54, 60, 54, 60};

	firewall_replay(pkts, 10, pkts_size);
}

static void test_firewall_icmp(void)
{
#	include "firewall-brick-icmp.c"
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
	g_test_add_func("/brick/firewall/filter", test_firewall_filter);
	g_test_add_func("/brick/firewall/tcp", test_firewall_tcp);
	g_test_add_func("/brick/firewall/icmp", test_firewall_icmp);
	g_test_add_func("/brick/firewall/noip", test_firewall_noip);
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

	test_firewall();
	r = g_test_run();

	packetgraph_stop();
	return r;
}
