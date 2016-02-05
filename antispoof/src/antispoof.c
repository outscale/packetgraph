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

#include <rte_config.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <packetgraph/brick.h>
#include <packetgraph/antispoof.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/utils/mac.h>

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

struct pg_antispoof_state {
	struct pg_brick brick;
	enum pg_side outside;
	struct ether_addr mac;
	uint32_t ip;
	struct pg_antispoof_arp arp_packet;
};

struct pg_antispoof_config {
	enum pg_side outside;
	struct ether_addr mac;
};

/* Size of the beginning of struct pg_antispoof_arp until ar_op (excluded) */
#define PG_ANTISPOOF_ARP_CHECK_PART1 (2 + 2 + 1 + 1)

/* Size of the "Sender" part of struct pg_antispoof_arp */
#define PG_ANTISPOOF_ARP_CHECK_PART2 (6 + 4)

void pg_antispoof_arp_enable(struct pg_brick *brick, uint32_t ip)
{
	struct pg_antispoof_state *state;

	state = pg_brick_get_state(brick, struct pg_antispoof_state);
	state->ip = ip;

	/* Let's adapt allowed ARP packets */
	state->arp_packet.sender_ip = ip;
}

void pg_antispoof_arp_disable(struct pg_brick *brick)
{
	struct pg_antispoof_state *state;

	state = pg_brick_get_state(brick, struct pg_antispoof_state);
	state->ip = 0;
}

static struct pg_brick_config *antispoof_config_new(const char *name,
						    uint32_t west_max,
						    uint32_t east_max,
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
	return pg_brick_config_init(config, name, west_max,
				    east_max, PG_MULTIPOLE);
}

static inline int antispoof_arp(struct pg_antispoof_state *state,
				struct pg_antispoof_arp *a)
{
	/* Check that all fields match reference packet except the "ar_op"
	 * (arp operation code) part.
	 */
	if (memcmp(&state->arp_packet, a, PG_ANTISPOOF_ARP_CHECK_PART1) == 0 &&
	    memcmp(&state->arp_packet.sender_mac, &a->sender_mac,
		   PG_ANTISPOOF_ARP_CHECK_PART2) == 0)
		return 1;
	return 0;
}

static int antispoof_burst(struct pg_brick *brick, enum pg_side from,
			   uint16_t edge_index, struct rte_mbuf **pkts,
			   uint16_t nb, uint64_t pkts_mask,
			   struct pg_error **errp)
{
	struct pg_antispoof_state *state;
	struct pg_brick_side *s;
	uint64_t it_mask;
	uint64_t bit;
	uint16_t i;

	state = pg_brick_get_state(brick, struct pg_antispoof_state);
	s = &brick->sides[pg_flip_side(from)];

	/* lets all packets from outside pass. */
	if (state->outside == from)
		return pg_brick_side_forward(s, from, pkts, nb,
					     pkts_mask, errp);

	/* packets come from inside, let's check few things */
	it_mask = pkts_mask;
	for (; it_mask;) {
		struct ether_hdr *eth;

		pg_low_bit_iterate_full(it_mask, bit, i);
		eth = rte_pktmbuf_mtod(pkts[i], struct ether_hdr*);

		/* MAC antispoof */
		if (memcmp(&eth->s_addr, &state->mac, ETHER_ADDR_LEN)) {
			pkts_mask &= ~bit;
			continue;
		}

		/* ARP antispoof */
		if ((eth->ether_type == htobe16(ETHER_TYPE_ARP)) && state->ip) {
			struct pg_antispoof_arp *a;

			a = (struct pg_antispoof_arp *)(eth + 1);
			if (!antispoof_arp(state, a)) {
				pkts_mask &= ~bit;
				continue;
			}
		}

		/* Drop RARP */
		if (eth->ether_type == htobe16(ETHER_TYPE_RARP)) {
			pkts_mask &= ~bit;
			continue;
		}
	}
	return pg_brick_side_forward(s, from, pkts, nb, pkts_mask, errp);
}

static int antispoof_init(struct pg_brick *brick,
			  struct pg_brick_config *config,
			  struct pg_error **errp)
{
	struct pg_antispoof_state *state;
	struct pg_antispoof_config *antispoof_config;

	state = pg_brick_get_state(brick, struct pg_antispoof_state);

	if (!config->brick_config) {
		*errp = pg_error_new("config->brick_config is NULL");
		return 0;
	}

	antispoof_config = (struct pg_antispoof_config *) config->brick_config;
	brick->burst = antispoof_burst;
	state->outside = antispoof_config->outside;
	memcpy(&state->mac, &antispoof_config->mac, ETHER_ADDR_LEN);
	state->ip = 0;

	/* let's start the pre-build of ARP anti-spoof */
	state->arp_packet.ar_hrd = htobe16(1);
	state->arp_packet.ar_pro = htobe16(ETHER_TYPE_IPv4);
	state->arp_packet.ar_hln = ETHER_ADDR_LEN;
	state->arp_packet.ar_pln = 4;
	memcpy(&state->arp_packet.sender_mac, &state->mac, ETHER_ADDR_LEN);

	if (pg_error_is_set(errp))
		return 0;

	return 1;
}

struct pg_brick *pg_antispoof_new(const char *name,
				  uint32_t west_max,
				  uint32_t east_max,
				  enum pg_side outside,
				  struct ether_addr mac,
				  struct pg_error **errp)
{
	struct pg_brick_config *config = antispoof_config_new(name, west_max,
							      east_max, outside,
							      mac);
	struct pg_brick *ret = pg_brick_new("antispoof", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static void antispoof_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_antispoof_state *state;

	state = pg_brick_get_state(brick, struct pg_antispoof_state);
	state->ip = 0;
}

static struct pg_brick_ops antispoof_ops = {
	.name		= "antispoof",
	.state_size	= sizeof(struct pg_antispoof_state),

	.init		= antispoof_init,
	.destroy	= antispoof_destroy,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(antispoof, &antispoof_ops);
