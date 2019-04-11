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

#define	__FAVOR_BSD
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#include <rte_config.h>
#include <rte_ether.h>
#include <pcap/pcap.h>

#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include <packetgraph/firewall.h>
#include "brick-int.h"
#include "collect.h"
#include "fail.h"
#include "packetsgen.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/mac.h"
#include "utils/bitmask.h"

struct header {
	struct ether_hdr eth;
	union {
		struct  __attribute__((__packed__)) {
			struct ip ip;
			uint16_t payload;
		} ip4;
		struct  __attribute__((__packed__)) {
			struct ip6_hdr ip;
			uint16_t payload;
		} ip6;
	}  __attribute__((__packed__));
}  __attribute__((__packed__));

#define build_ip(ip, eth, src_ip, dst_ip, af) do {		\
		ip = &hdr->ip4.ip;				\
		ip->ip_v = IPVERSION;				\
		ip->ip_hl = sizeof(struct ip) >> 2;		\
		ip->ip_off = 0;					\
		ip->ip_ttl = 64;				\
		ip->ip_p = 16; /* FEAR ! this is CHAOS ! */	\
		ip->ip_len = htons(sizeof(struct ip) + 1);	\
		inet_pton(af, src_ip, &ip->ip_src.s_addr);	\
		inet_pton(af, dst_ip, &ip->ip_dst.s_addr);	\
	} while (0)

#define build_ip6(ipv6, hdr, src_ip, dst_ip, af) do {			\
		ipv6 = &hdr->ip6.ip;					\
		ipv6->ip6_flow = 0;					\
		ipv6->ip6_vfc = 6 << 4;					\
		ipv6->ip6_nxt = 16; /* FEAR ! this is CHAOS ! */	\
		ipv6->ip6_plen = sizeof(uint16_t);			\
		ipv6->ip6_hlim = 64;					\
		inet_pton(AF_INET6, src_ip, &ipv6->ip6_src.s6_addr);	\
		inet_pton(AF_INET6, dst_ip, &ipv6->ip6_dst.s6_addr);	\
	} while (0)

static struct rte_mbuf *build_ip_packet(const char *src_ip,
					const char *dst_ip, uint16_t data,
					int af)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mp);
	uint16_t len = af == AF_INET ? sizeof(struct ether_hdr) +
		sizeof(struct ip) +
		sizeof(uint16_t) : sizeof(struct ether_hdr) +
		sizeof(struct ip6_hdr) +
		sizeof(uint16_t);
	struct header *hdr;
	struct ether_hdr *eth;
	struct ip *ip;
	struct ip6_hdr *ipv6;
	uint16_t *payload_ip;

	pkt->pkt_len = len;
	pkt->data_len = len;
	pkt->nb_segs = 1;
	pkt->l2_len = sizeof(struct ether_hdr);
	pkt->next = NULL;

	hdr = rte_pktmbuf_mtod(pkt, struct header *);
	/* ethernet header */
	eth = &hdr->eth;
	memset(eth, 0, len);

	/* ipv4 header */
	if (af == AF_INET6) {
		eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv6);
		build_ip6(ipv6, hdr, src_ip, dst_ip, af);
		payload_ip = &hdr->ip6.payload;
	} else {
		eth->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
		build_ip(ip, hdr, src_ip, dst_ip, af);
		payload_ip = &hdr->ip4.payload;
	}

	/* write some data */
	*payload_ip = data;
	return pkt;
}

static struct rte_mbuf *build_packet(const unsigned char *data, size_t len)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt = rte_pktmbuf_alloc(mp);
	char *packet;

	g_assert(pkt);
	pkt->l2_len = sizeof(struct ether_hdr);

	packet = rte_pktmbuf_append(pkt, len);
	g_assert(packet);
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

/* send burst, get mask after firewall */
static uint64_t firewall_scenario_filter(const char *rule,
					 struct rte_mbuf **pkts,
					 uint64_t mask)
{
	struct pg_brick *gen, *fw, *col;
	struct pg_error *error = NULL;
	uint64_t filtered_mask;
	int nb = pg_mask_count(mask);
	uint16_t packet_count;

	/* create and connect 3 bricks: generator -> firewall -> collector */
	gen = pg_packetsgen_new("gen", 2, 2, PG_EAST_SIDE, pkts, nb, &error);
	g_assert(!error);
	fw = pg_firewall_new("fw", PG_NONE, &error);
	g_assert(!error);
	col = pg_collect_new("col", &error);
	g_assert(!error);

	pg_brick_chained_links(&error, gen, fw, col);
	g_assert(!error);

	if (rule) {
		g_assert(!pg_firewall_rule_add(fw, rule, PG_WEST_SIDE, 0,
					       &error));
		g_assert(!error);
		g_assert(!pg_firewall_reload(fw, &error));
		g_assert(!error);
	}

	/* let's burst ! */
	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);

	/* check collect brick */
	pg_brick_west_burst_get(col, &filtered_mask, &error);
	g_assert(!error);

	pg_brick_destroy(gen);
	pg_brick_destroy(fw);
	pg_brick_destroy(col);
	return filtered_mask;
}

static struct rte_mbuf **test_filtre(struct pg_brick *gen, struct pg_brick *col,
				     int expected_packets,
				     int nb, enum pg_side dir,
				     uint64_t *filtered_pkts_mask)
{
	uint16_t packet_count;
	struct pg_error *error = NULL;
	struct rte_mbuf **filtered_pkts;

	pg_brick_poll(gen, &packet_count, &error);
	g_assert(!error);
	g_assert(packet_count == nb);

	/* check collect brick */
	if (dir == PG_WEST_SIDE)
		filtered_pkts = pg_brick_west_burst_get(col,
							filtered_pkts_mask,
							&error);
	else
		filtered_pkts = pg_brick_east_burst_get(col,
							filtered_pkts_mask,
							&error);
	g_assert(!error);
	g_assert(pg_mask_count(*filtered_pkts_mask) == expected_packets);
	return filtered_pkts;
}

static void firewall_filter_rules(enum pg_side dir)
{
	struct pg_brick *gen;
	struct pg_brick *fw;
	struct pg_brick *col;
	struct pg_error *error = NULL;
	uint16_t i;
	static uint16_t nb = 30;
	struct rte_mbuf *packets[30];
	uint64_t filtered_pkts_mask;
	struct rte_mbuf **filtered_pkts;
	uint64_t bit;
	struct ip *ip;
	struct ip6_hdr *ipv6;
	struct ether_hdr *eth;

	/* create and connect 3 bricks: generator -> firewall -> collector */
	gen = pg_packetsgen_new("gen", 2, 2, pg_flip_side(dir),
				packets, nb, &error);
	g_assert(!error);
	fw = pg_firewall_new("fw", PG_NONE, &error);
	g_assert(!error);
	col = pg_collect_new("col", &error);
	g_assert(!error);

	/* revert link if needed */
	if (dir == PG_WEST_SIDE) {
		pg_brick_chained_links(&error, gen, fw, col);
		g_assert(!error);
	} else {
		pg_brick_chained_links(&error, col, fw, gen);
		g_assert(!error);
	}

	/* build some UDP packets mixed sources */
	for (i = 0; i < nb; i++)
		switch (i % 3) {
		case 0:
			packets[i] = build_ip_packet("10.0.0.1",
						     "10.0.0.255", i, AF_INET);
			break;
		case 1:
			packets[i] = build_ip_packet("10::2",
						     "10::255", i, AF_INET6);
			break;
		case 2:
			packets[i] = build_ip_packet("10.0.0.3",
						     "10.0.0.255", i, AF_INET);
			break;
		}

	/* configure firewall to allow traffic from 10.0.0.1 */
	g_assert(!pg_firewall_rule_add(fw, "src host 10.0.0.1", dir, 0,
				       &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let's burst ! */
	filtered_pkts = test_filtre(gen, col, nb / 3, nb, dir,
				    &filtered_pkts_mask);
	for (; filtered_pkts_mask;) {
		uint32_t tmp;

		pg_low_bit_iterate(filtered_pkts_mask, i);
		g_assert(i % 3 == 0);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);

		inet_pton(AF_INET, "10.0.0.1", &tmp);
		g_assert(ip->ip_src.s_addr == tmp);
	}

	/* now allow packets from 10::2 */
	g_assert(!pg_firewall_rule_add(fw, "src host 10::2", dir, 0,
				       &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let it goooo */
	filtered_pkts = test_filtre(gen, col, nb * 2 / 3, nb, dir,
				    &filtered_pkts_mask);
	for (; filtered_pkts_mask;) {
		uint32_t tmp1;
		uint8_t tmp2[16];

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0 || i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);
		ipv6 = (struct ip6_hdr *)(eth + 1);
		inet_pton(AF_INET, "10.0.0.1", &tmp1);
		inet_pton(AF_INET6, "10::2", &tmp2);
		g_assert(ip->ip_src.s_addr == tmp1 ||
			 !memcmp(ipv6->ip6_src.s6_addr, tmp2, 16));
	}

	/* test that flush really blocks */
	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);
	g_assert(pg_brick_reset(col, &error) >= 0);
	g_assert(!error);

	/* let it goooo */
	test_filtre(gen, col, 0, nb, dir, &filtered_pkts_mask);
	/* flush and only allow packets from 10::2 */
	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_rule_add(fw, "src host 10::2",
				       dir, 0, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let it goooo */
	filtered_pkts = test_filtre(gen, col, nb / 3, nb, dir,
				    &filtered_pkts_mask);
	for (; filtered_pkts_mask;) {
		uint8_t tmp[16];

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ipv6 = (struct ip6_hdr *)(eth + 1);
		inet_pton(AF_INET6, "10::2", &tmp);
		g_assert(!memcmp(ipv6->ip6_src.s6_addr, tmp, 16));

	}

	/* flush and make two rules in one */
	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_rule_add(fw, "src host (10.0.0.1 or 10::2)",
				       dir, 0, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* let it goooo */
	filtered_pkts = test_filtre(gen, col, nb * 2 / 3, nb,
				    dir, &filtered_pkts_mask);
	for (; filtered_pkts_mask;) {
		uint32_t tmp1;
		uint8_t tmp2[16];

		pg_low_bit_iterate_full(filtered_pkts_mask, bit, i);
		g_assert(i % 3 == 0 || i % 3 == 1);
		eth = rte_pktmbuf_mtod(filtered_pkts[i], struct ether_hdr*);
		ip = (struct ip *)(eth + 1);
		ipv6 = (struct ip6_hdr *)(eth + 1);
		inet_pton(AF_INET, "10.0.0.1", &tmp1);
		inet_pton(AF_INET6, "10::2", &tmp2);
		g_assert(ip->ip_src.s_addr == tmp1 ||
			 !memcmp(ipv6->ip6_src.s6_addr, tmp2, 16));
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
	test_filtre(gen, col, 0, nb, dir, &filtered_pkts_mask);

	pg_firewall_rule_flush(fw);
	g_assert(!pg_firewall_rule_add(fw, "src host (10.0.0.1)",
				       dir, 0, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	/* check that flushing other side don't remove the rule */
	pg_firewall_rule_flush_side(fw, pg_flip_side(dir));
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);
	g_assert(pg_brick_reset(col, &error) >= 0);
	g_assert(!error);
	filtered_pkts = test_filtre(gen, col, nb / 3, nb,
				    dir, &filtered_pkts_mask);

	/* check that flushing side remove the rule */
	pg_firewall_rule_flush_side(fw, dir);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);
	g_assert(pg_brick_reset(col, &error) >= 0);
	g_assert(!error);
	test_filtre(gen, col, 0, nb, dir, &filtered_pkts_mask);

	/* allow packets from both sides */
	g_assert(!pg_firewall_rule_add(fw, "src host (10.0.0.1)",
				       PG_MAX_SIDE, 0, &error));
	g_assert(!error);
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);
	filtered_pkts = test_filtre(gen, col, nb / 3, nb,
				    dir, &filtered_pkts_mask);
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
	if (dir == PG_WEST_SIDE) {
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
	filtered_pkts = test_filtre(gen, col, nb / 3, nb, dir,
				    &filtered_pkts_mask);
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
			    int pkts_nb, int *pkts_size, int nb_firewalls)
{
	struct pg_brick *gen_west, *gen_east;
	/* Well most compiller handle VLA */
	struct pg_brick *fw[nb_firewalls];
	struct pg_brick *hub_east, *hub_west;
	struct pg_brick *col_west, *col_east;
	struct pg_error *error = NULL;
	uint16_t packet_count;
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
	gen_west = pg_packetsgen_new("gen_west", 1, 1, PG_EAST_SIDE, &packet, 1,
				  &error);
	g_assert(!error);
	gen_east = pg_packetsgen_new("gen_east", 1, 1, PG_WEST_SIDE, &packet, 1,
				  &error);
	g_assert(!error);
	hub_west = pg_hub_new("west_hub", 1, nb_firewalls, &error);
	g_assert(!error);
	pg_hub_set_no_backward(hub_west, 1);
	for (int i = 0; i < nb_firewalls; ++i) {
		fw[i] = pg_firewall_new("fw", PG_NONE, &error);
		g_assert(!error);
	}
	hub_east = pg_hub_new("east_hub", nb_firewalls, 1, &error);
	g_assert(!error);
	pg_hub_set_no_backward(hub_east, 1);
	col_west = pg_collect_new("col_west", &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	pg_brick_chained_links(&error, col_west, gen_west, hub_west);
	g_assert(!error);
	for (int i = 0; i < nb_firewalls; ++i) {
		pg_brick_chained_links(&error, hub_west, fw[i], hub_east);
		g_assert(!error);
	}
	pg_brick_chained_links(&error, hub_east, gen_east, col_east);
	g_assert(!error);

	/* open all traffic of 10.0.2.15 from the west side of the firewall
	 * returning traffic should be allowed due to STATEFUL option
	 */
	for (int i = 0; i < nb_firewalls; ++i) {
		g_assert(!pg_firewall_rule_add(fw[i], "src host 10.0.2.15",
					       PG_WEST_SIDE, 1, &error));
		g_assert(!error);
		g_assert(!pg_firewall_reload(fw[i], &error));
		g_assert(!error);
	}

	/* replay traffic */
	uint64_t west_burst = 0;
	uint64_t east_burst = 0;

	for (int i = 0; i < pkts_nb; i++) {
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

			++east_burst;
			pg_brick_poll(gen_west, &packet_count, &error);
			g_assert(!error);
			g_assert(packet_count == 1);
			filtered_pkts = pg_brick_west_burst_get(col_east,
				&filtered_pkts_mask, &error);
			g_assert(!error);
			g_assert(pg_mask_count(filtered_pkts_mask) == 1);
			g_assert(pg_brick_pkts_count_get(col_east,
							 PG_EAST_SIDE) ==
				 east_burst * nb_firewalls);
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

			++west_burst;
			pg_brick_poll(gen_east, &packet_count, &error);
			g_assert(!error);
			g_assert(packet_count == 1);
			filtered_pkts = pg_brick_east_burst_get(col_west,
				&filtered_pkts_mask, &error);
			g_assert(!error);
			g_assert(pg_brick_pkts_count_get(col_west,
							 PG_WEST_SIDE) ==
				 west_burst * nb_firewalls);

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
		for (int j = 0; j < nb_firewalls; ++j) {
			g_assert(!pg_firewall_rule_add(fw[j],
						       "src host 6.6.6.6",
						       PG_WEST_SIDE, 0,
						       &error));
			g_assert(!error);
			g_assert(!pg_firewall_reload(fw[j], &error));
			g_assert(!error);
		}
	}

	/* clean */
	pg_brick_destroy(gen_west);
	pg_brick_destroy(gen_east);
	pg_brick_destroy(col_west);
	pg_brick_destroy(col_east);
	pg_brick_destroy(hub_west);
	pg_brick_destroy(hub_east);
	for (int i = 0; i < nb_firewalls; ++i)
		pg_brick_destroy(fw[i]);
}

static void test_firewall_noip(void)
{
	uint16_t i;
	int nb = 30;
	struct rte_mbuf *pkts[30];
	uint64_t mask = pg_mask_firsts(nb);

	/* build some non-IP packets */
	for (i = 0; i < nb; i++)
		pkts[i] = build_non_ip_packet();

	g_assert(firewall_scenario_filter(NULL, pkts, mask) == mask);

	/* clean */
	for (i = 0; i < nb; i++)
		rte_pktmbuf_free(pkts[i]);
}

static void test_firewall_filter(void)
{
	firewall_filter_rules(PG_WEST_SIDE);
	firewall_filter_rules(PG_EAST_SIDE);
}

static void test_firewall_tcp(void)
{
#	include "test-tcp.c"
	const unsigned char *pkts[] = {pkt1, pkt2, pkt3, pkt4, pkt5,
		pkt6, pkt7, pkt8, pkt9, pkt10};
	int pkts_size[] = {74, 60, 54, 58, 60, 575, 54, 60, 54, 60};

	firewall_replay(pkts, 10, pkts_size, 10);
}

static void test_firewall_tcp6(void)
{
#       include "test-tcp6.c"
	const unsigned char *pkts = pkt1;
	int pkt_size = 94;
	uint64_t mask = pg_mask_firsts(1);
	struct rte_mbuf *packet = build_packet(pkts, pkt_size);
	const char *rule =
		"src net 2001:db8:2000:aff0::1/128 and tcp dst port 8000";
	g_assert(firewall_scenario_filter(rule, &packet, mask) == mask);
}

static void test_firewall_exotic(void)
{
	int nb = 2;
	uint64_t mask = pg_mask_firsts(nb);
	struct rte_mbuf *pkts[] = {
		build_ip_packet("10.0.0.1", "10.0.0.255", 42, AF_INET),
		build_ip_packet("10::2", "10::255", 42, AF_INET6)
	};

	/* FIXME ipv6 protochain not working here */
	const char *rule = "(ip proto 16) or (ip6 proto 16)";

	g_assert(firewall_scenario_filter(rule, pkts, mask) == mask);

	for (int i = 0; i < nb; i++)
		rte_pktmbuf_free(pkts[i]);
}

static void test_firewall_empty_rules(void)
{
	/* no rules, should not let packets pass */
	struct rte_mbuf *pkt =
		build_ip_packet("10.0.0.1", "10.0.0.255", 42, AF_INET);
	g_assert(firewall_scenario_filter(NULL, &pkt, pg_mask_firsts(1)) == 0);
	rte_pktmbuf_free(pkt);
}

static void test_firewall_icmp(void)
{
#	include "test-icmp.c"
	const unsigned char *pkts[] = {pkt1, pkt2, pkt3, pkt4, pkt5,
		pkt6, pkt7, pkt8, pkt9, pkt10};
	int pkts_size[] = {74, 60, 54, 58, 60, 571, 54, 60, 54, 60};

	firewall_replay(pkts, 10, pkts_size, 1);
}

static void test_firewall_rules(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *fw = pg_firewall_new("fw", PG_NONE, &error);
	static const char * const r[] = {
		"src host 10.0.0.1",
		"udp",
		"tcp",
		"icmp",
		"icmp or udp or tcp",
		"udp dst portrange 4000-6000",
		"tcp dst portrange 0-65535",
		"tcp dst port 42",
		"  ",
		"",
		"src host 10.0.0.1 and (icmp or udp or tcp)",
		"((src net 0.0.0.0/0 and udp dst portrange 4000-6000))",
		"((src net 0.0.0.0/0 && udp dst portrange 4000-6000))",
		"((src net 0.0.0.0/0 or udp dst portrange 4000-6000))",
		"((src net 0.0.0.0/0 || udp dst portrange 4000-6000))",
		NULL
	};

	for (int i = 0; r[i]; i++) {
		g_assert(!pg_firewall_rule_add(fw, r[i], PG_WEST_SIDE,
					       0, &error));
		g_assert(!pg_firewall_reload(fw, &error));
		g_assert(!error);
		pg_firewall_rule_flush(fw);
		g_assert(!pg_firewall_reload(fw, &error));
		g_assert(!error);
	}
	pg_brick_destroy(fw);
}

static void test_firewall_empty_burst(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *fw;
	struct pg_brick *fail;
	struct rte_mbuf **pkts;
	struct ether_addr eth;

	pg_scan_ether_addr(&eth, "00:18:b9:56:2e:73");

	fw = pg_firewall_new("fw", PG_NONE, &error);
	g_assert(!error);
	fail = pg_fail_new("fail", &error);
	g_assert(!error);

	pg_brick_link(fw, fail, &error);
	g_assert(!error);

	g_assert(!pg_firewall_rule_add(fw, "src host 10.0.2.15",
				       PG_EAST_SIDE, 0, &error));
	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);

	pkts = pg_packets_append_ether(pg_packets_create(pg_mask_firsts(64)),
				       pg_mask_firsts(64), &eth, &eth,
				       ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, pg_mask_firsts(32), 1, 2, 0, 0);
	pg_brick_burst_to_east(fw, 0, pkts, pg_mask_firsts(64), &error);
	g_assert(!error);

	pg_brick_destroy(fw);
	pg_brick_destroy(fail);
	pg_packets_free(pkts, pg_mask_firsts(64));
	g_free(pkts);
}

static void test_firewall_lifecyle(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *fw = pg_firewall_new("fw", PG_NONE, &error);

	g_assert(!error);
	pg_brick_destroy(fw);
	fw = pg_firewall_new("fw", PG_NONE, &error);

	g_assert(!pg_firewall_reload(fw, &error));
	g_assert(!error);
	pg_brick_destroy(fw);
}

static void test_firewall(void)
{
	pg_test_add_func("/firewall/lifecycle", test_firewall_lifecyle);
	pg_test_add_func("/firewall/filter", test_firewall_filter);
	pg_test_add_func("/firewall/tcp", test_firewall_tcp);
	pg_test_add_func("/firewall/icmp", test_firewall_icmp);
	pg_test_add_func("/firewall/noip", test_firewall_noip);
	pg_test_add_func("/firewall/rules", test_firewall_rules);
	pg_test_add_func("/firewall/empty_burst", test_firewall_empty_burst);
	pg_test_add_func("/firewall/tcp6", test_firewall_tcp6);
	pg_test_add_func("/firewall/exotic", test_firewall_exotic);
	pg_test_add_func("/firewall/empty_rules", test_firewall_empty_rules);
}

int main(int argc, char **argv)
{
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	g_assert(pg_start(argc, argv) >= 0);
	g_assert(!pg_init_seccomp());

	test_firewall();
	int r = g_test_run();

	pg_stop();
	return r;
}
