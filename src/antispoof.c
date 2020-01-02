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

#include <rte_config.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_memcpy.h>
#include <packetgraph/packetgraph.h>
#include "brick-int.h"
#include "utils/bitmask.h"
#include "utils/mac.h"
#include "utils/ip.h"
#include "utils/network.h"
#include <packetgraph/antispoof.h>

struct pg_antispoof_arp {
	/* Format of hardware address.  */
	uint16_t ar_hrd;
	/* Format of protocol address.  */
	uint16_t ar_pro;
	/* Length of hardware address.  */
	uint8_t ar_hln;
	/* Length of protocol address.  */
	uint8_t ar_pln;
	/* ARP opcode (command). */
	uint16_t ar_op;
	/* Sender hardware address. */
	uint8_t sender_mac[ETHER_ADDR_LEN];
	/* Sender IP address. */
	uint32_t sender_ip;
	/* Target hardware address. */
	uint8_t target_mac[ETHER_ADDR_LEN];
	/* Target IP address. */
	uint32_t target_ip;
} __attribute__ ((__packed__));

struct arp {
	uint32_t ip;
	struct pg_antispoof_arp packet;
};

struct opt_ll_addr {
	/* 1 for Source Link-layer Address
	 * 2 for Target Link-layer Address (X)
	 */
	uint8_t type;
	/* IEEE 802 addresses is 1. */
	uint8_t len;
	struct ether_addr addr;
} __attribute__ ((__packed__));

struct neighbor_advertisement {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint8_t router_flag:1;
	uint8_t solicited_flag:1;
	uint8_t override_flag:1;
	uint32_t reserved:29;
	uint8_t target_address[16];
	/* link layer address option */
	struct opt_ll_addr ll;
} __attribute__ ((__packed__));

struct ndp {
	uint8_t ip[16];
};

struct pg_antispoof_state {
	struct pg_brick brick;
	enum pg_side outside;
	struct ether_addr mac;
	bool arp_enabled;
	uint16_t arps_size;
	struct arp arps[PG_ARP_MAX];
	/* icmpv6 / neighbor discovery */
	bool ndp_enabled;
	uint16_t ndps_size;
	struct ndp ndps[PG_NPD_MAX];
};

struct pg_antispoof_config {
	enum pg_side outside;
	struct ether_addr mac;
};

/* Size of the beginning of struct pg_antispoof_arp until ar_op (excluded) */
#define PG_ANTISPOOF_ARP_CHECK_PART1 (2 + 2 + 1 + 1)

/* Size of the "Sender" part of struct pg_antispoof_arp */
#define PG_ANTISPOOF_ARP_CHECK_PART2 (6 + 4)

void pg_antispoof_arp_enable(struct pg_brick *brick)
{
	pg_brick_get_state(brick, struct pg_antispoof_state)->arp_enabled =
		true;
}

int pg_antispoof_arp_add(struct pg_brick *brick, uint32_t ip,
			 struct pg_error **errp)
{
	struct pg_antispoof_state *state =
		pg_brick_get_state(brick, struct pg_antispoof_state);
	uint16_t n = state->arps_size;
	struct arp *arp = &state->arps[n];

	if (unlikely(n == PG_ARP_MAX)) {
		*errp = pg_error_new("Maximal IP reached");
		return -1;
	}

	/* let's start the pre-build of ARP anti-spoof */
	arp->packet.ar_hrd = rte_cpu_to_be_16(1);
	arp->packet.ar_pro = PG_BE_ETHER_TYPE_IPv4;
	arp->packet.ar_hln = ETHER_ADDR_LEN;
	arp->packet.ar_pln = 4;
	arp->packet.sender_ip = ip;
	rte_memcpy(&arp->packet.sender_mac, &state->mac, ETHER_ADDR_LEN);

	arp->ip = ip;

	state->arps_size++;
	return 0;
}

int pg_antispoof_arp_del(struct pg_brick *brick, uint32_t ip,
			 struct pg_error **errp)
{
	struct pg_antispoof_state *state =
		pg_brick_get_state(brick, struct pg_antispoof_state);
	int index = -1;
	uint16_t size = state->arps_size;
	struct arp *arps = state->arps;

	for (int16_t i = 0; i < size; i++) {
		if (arps[i].ip == ip) {
			index = i;
			break;
		}
	}

	if (index < 0) {
		*errp = pg_error_new("IP not found");
		return -1;
	}

	if (index != size - 1)
		rte_memcpy(&arps[index], &arps[size - 1], sizeof(struct arp));
	state->arps_size--;
	return 0;
}

void pg_antispoof_arp_del_all(struct pg_brick *brick)
{
	struct pg_antispoof_state *state =
		pg_brick_get_state(brick, struct pg_antispoof_state);
	state->arps_size = 0;
}

void pg_antispoof_arp_disable(struct pg_brick *brick)
{
	pg_brick_get_state(brick, struct pg_antispoof_state)->arp_enabled =
		false;
}

static struct pg_brick_config *antispoof_config_new(const char *name,
						    enum pg_side outside,
						    struct ether_addr mac)
{
	struct pg_brick_config *config;
	struct pg_antispoof_config *antispoof_config;

	antispoof_config = g_new0(struct pg_antispoof_config, 1);
	antispoof_config->outside = outside;
	antispoof_config->mac = mac;
	config = g_new0(struct pg_brick_config, 1);
	config->brick_config = (void *) antispoof_config;
	return pg_brick_config_init(config, name, 1, 1, PG_DIPOLE);
}

static inline int antispoof_arp(struct pg_antispoof_state *state,
				struct rte_mbuf *pkt)
{
	uint16_t size = state->arps_size;
	struct pg_antispoof_arp *p;
	struct pg_antispoof_arp *a = pg_utils_get_l3(pkt);

	for (uint16_t i = 0; i < size; i++) {
		/* Check that all fields match reference packet except the
		 * "ar_op" (arp operation code) part.
		 */
		p = &state->arps[i].packet;
		if (!memcmp(p, a, PG_ANTISPOOF_ARP_CHECK_PART1) &&
		    memcmp(&p->sender_mac, &a->sender_mac,
			   PG_ANTISPOOF_ARP_CHECK_PART2) == 0) {
			return 0;
		}
	}
	return -1;
}

void pg_antispoof_ndp_enable(struct pg_brick *brick)
{
	pg_brick_get_state(brick, struct pg_antispoof_state)->ndp_enabled =
		true;
}

int pg_antispoof_ndp_add(struct pg_brick *brick, uint8_t *ip,
			struct pg_error **errp)
{
	struct pg_antispoof_state *state =
		pg_brick_get_state(brick, struct pg_antispoof_state);
	uint16_t n = state->ndps_size;
	struct ndp *ndp = &state->ndps[n];

	if (unlikely(n == PG_NPD_MAX)) {
		*errp = pg_error_new("Maximal IPV6 reached");
		return -1;
	}

	pg_ip_copy(ip, ndp->ip);
	state->ndps_size++;
	return 0;

}

int pg_antispoof_ndp_del(struct pg_brick *brick, uint8_t *ip,
			struct pg_error **errp)
{
	struct pg_antispoof_state *state =
		pg_brick_get_state(brick, struct pg_antispoof_state);
	int index = -1;
	uint16_t size = state->ndps_size;
	struct ndp *ndps = state->ndps;

	for (int16_t i = 0; i < size; i++) {
		if (pg_ip_is_same(ndps[i].ip, ip)) {
			index = i;
			break;
		}
	}

	if (index < 0) {
		*errp = pg_error_new("IPV6 not found");
		return -1;
	}

	if (index != size - 1)
		pg_ip_copy(ndps[size - 1].ip, ndps[index].ip);
	state->ndps_size--;
	return 0;
}

void pg_antispoof_ndp_del_all(struct pg_brick *brick)
{
	pg_brick_get_state(brick, struct pg_antispoof_state)->ndps_size = 0;
}

void pg_antispoof_ndp_disable(struct pg_brick *brick)
{
	pg_brick_get_state(brick, struct pg_antispoof_state)->ndp_enabled =
		false;
}

static inline int antispoof_ndp(struct pg_antispoof_state *state,
				struct rte_mbuf *pkt)
{
	uint8_t ipv6_type;
	uint8_t *ipv6_payload;

	if (pg_utils_get_ipv6_l4(pkt, &ipv6_type, &ipv6_payload) < 0)
		return -1;
	if (likely(ipv6_type != PG_IP_TYPE_ICMPV6))
		return 0;

	/* check ICMPv6 message type */
	struct neighbor_advertisement *na =
		(struct neighbor_advertisement *) ipv6_payload;

	if (likely(na->type != PG_ICMPV6_TYPE_NA))
		return 0;

	/* check ipv6 source validity */
	uint16_t size = state->ndps_size;
	struct ndp *ndps = state->ndps;
	bool ipv6_allowed = false;
	struct ipv6_hdr *h6 = (struct ipv6_hdr *) pg_utils_get_l3(pkt);

	for (uint16_t i = 0; i < size; i++) {
		if (pg_ip_is_same(h6->src_addr, ndps[i].ip) &&
		    !memcmp(na->target_address, ndps[i].ip, 16)) {
			ipv6_allowed = true;
			break;
		}
	}
	if (unlikely(!ipv6_allowed))
		return -1;

	/* check link layer address option */
	if (unlikely(na->ll.type != 2 || na->ll.len != 1 ||
		     memcmp(&na->ll.addr, &state->mac, ETHER_ADDR_LEN))) {
		return -1;
	}

	return 0;
}

static int antispoof_burst(struct pg_brick *brick, enum pg_side from,
			   uint16_t edge_index, struct rte_mbuf **pkts,
			   uint64_t pkts_mask,
			   struct pg_error **errp)
{
	struct pg_antispoof_state *state;
	struct pg_brick_side *s;
	struct ether_hdr *eth;
	uint16_t etype;
	uint64_t it_mask;
	uint64_t bit;
	uint16_t i;

	state = pg_brick_get_state(brick, struct pg_antispoof_state);
	s = &brick->sides[pg_flip_side(from)];

	/* lets all packets from outside pass. */
	if (state->outside == from)
		goto forward;

	/* packets come from inside, let's check few things */
	it_mask = pkts_mask;
	for (; it_mask;) {
		pg_low_bit_iterate_full(it_mask, bit, i);
		eth = rte_pktmbuf_mtod(pkts[i], struct ether_hdr*);
		etype = pg_utils_get_ether_type(pkts[i]);

		/* MAC antispoof */
		if (unlikely(memcmp(&eth->s_addr, &state->mac,
				    ETHER_ADDR_LEN)))
			pkts_mask &= ~bit;
		else if (unlikely(etype == PG_BE_ETHER_TYPE_RARP))
			pkts_mask &= ~bit;
		else if (state->arp_enabled &&
			 unlikely(etype == PG_BE_ETHER_TYPE_ARP) &&
			 antispoof_arp(state, pkts[i]) < 0)
			pkts_mask &= ~bit;
		else if (state->ndp_enabled &&
			 unlikely(etype == PG_BE_ETHER_TYPE_IPv6) &&
			 antispoof_ndp(state, pkts[i]) < 0)
			pkts_mask &= ~bit;
	}
	if (unlikely(pkts_mask == 0))
		return 0;
forward:
	return pg_brick_burst(s->edge.link, from, s->edge.pair_index,
			      pkts, pkts_mask, errp);
}

static int antispoof_init(struct pg_brick *brick,
			  struct pg_brick_config *config,
			  struct pg_error **errp)
{
	struct pg_antispoof_state *state;
	struct pg_antispoof_config *antispoof_config;

	state = pg_brick_get_state(brick, struct pg_antispoof_state);

	antispoof_config = (struct pg_antispoof_config *) config->brick_config;
	brick->burst = antispoof_burst;
	state->outside = antispoof_config->outside;
	rte_memcpy(&state->mac, &antispoof_config->mac, ETHER_ADDR_LEN);
	state->arp_enabled = false;
	state->arps_size = 0;
	return 0;
}

struct pg_brick *pg_antispoof_new(const char *name,
				  enum pg_side outside,
				  struct ether_addr *mac,
				  struct pg_error **errp)
{
	struct pg_brick_config *config;
	struct pg_brick *ret;

	g_assert(mac);
	config = antispoof_config_new(name, outside, *mac);
	ret = pg_brick_new("antispoof", config, errp);
	pg_brick_config_free(config);
	return ret;
}

static struct pg_brick_ops antispoof_ops = {
	.name		= "antispoof",
	.state_size	= sizeof(struct pg_antispoof_state),
	.init		= antispoof_init,
	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(antispoof, &antispoof_ops);

