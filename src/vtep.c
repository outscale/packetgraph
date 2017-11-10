/* Copyright 2015 Nodalink EURL
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
 *
 * The multicast_subscribe and multicast_unsubscribe functions
 * are modifications of igmpSendReportMessage/igmpSendLeaveGroupMessage from
 * CycloneTCP Open project.
 * Copyright 2010-2015 Oryx Embedded SARL.(www.oryx-embedded.com)
 */

/**
 * Implements: https://tools.ietf.org/html/rfc7348#section-4.1
 *
 * Note: the implementation does not support the optional VLAN tag in the VXLAN
 *	 header
 *
 * Note2: the implementation expects that the IP checksum will be offloaded to
 *	  the NIC
 */

#include <rte_config.h>
#include <rte_errno.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_memcpy.h>
#include <rte_prefetch.h>
#include <rte_udp.h>
#include <rte_tcp.h>
#include <arpa/inet.h>
#include <errno.h>

#include <packetgraph/common.h>
#include <packetgraph/vtep.h>
#include <packetgraph/errors.h>

#include "brick-int.h"
#include "packets.h"
#include "utils/bitmask.h"
#include "utils/mempool.h"
#include "utils/mac-table.h"
#include "utils/mac.h"
#include "utils/network.h"
#include "utils/ip.h"

#include "vtep-internal.h"

static void multicast4_subscribe(struct vtep_state *state,
				 struct vtep_port *port,
				 uint32_t multicast_ip,
				 struct pg_error **errp);

static void multicast4_unsubscribe(struct vtep_state *state,
				   struct vtep_port *port,
				   uint32_t multicast_ip,
				   struct pg_error **errp);

#define IGMP_PROTOCOL_NUMBER 0x02

#define IP_VERSION 4
#include "vtep-internal.c"
#undef IP_VERSION

struct igmp_hdr {
	uint8_t type;
	uint8_t max_resp_time;
	uint16_t checksum;
	uint32_t group_addr;
} __attribute__((__packed__));

struct multicast_pkt {
	struct ether_hdr ethernet;
	struct ipv4_hdr ipv4;
	struct {
		uint8_t type;
		uint8_t size;
		uint16_t unused;
	} __attribute__((__packed__)) ip_option;
	struct igmp_hdr igmp;
} __attribute__((__packed__));

#define IGMP_PKT_LEN sizeof(struct multicast_pkt)


struct pg_brick *pg_vtep_new_by_string(const char *name, uint32_t max,
				       enum pg_side output, const char *ip,
				       struct ether_addr mac,
				       uint16_t udp_dst_port,
				       int flag, struct pg_error **errp)
{
	uint32_t ip4;
	uint8_t ip6[16];

	if (pg_ip_from_str(ip4, ip) < 1) {
		if (pg_ip_from_str(ip6, ip) < 1) {
			*errp = pg_error_new("'%s' is not a valid ip", ip);
			return NULL;
		}
		return pg_vtep_new(name, max, output, ip6, mac, udp_dst_port,
				   flag, errp);
	}
	return pg_vtep_new(name, max, output, ip4, mac, udp_dst_port, flag,
			   errp);
}

inline struct ether_addr *pg_vtep_get_mac(const struct pg_brick *brick)
{
	if (unlikely(!brick))
		return NULL;
	return &pg_brick_get_state(brick, struct vtep_state)->mac;
}

static inline void do_add_mac(struct vtep_port *port, struct ether_addr *mac);

static inline uint16_t igmp_checksum(struct igmp_hdr *msg)
{
	uint16_t sum = 0;

	sum = rte_raw_cksum(msg, sizeof(struct igmp_hdr));
	return ~sum;
}

static void multicast4_internal(struct vtep_state *state,
				struct vtep_port *port,
				uint32_t multicast_ip,
				enum operation action,
				struct pg_error **errp)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt[1];
	struct multicast_pkt *hdr;
	struct ether_addr dst = pg_multicast_get_dst_addr(multicast_ip);

	if (!pg_is_multicast_ip(multicast_ip)) {
		char tmp[20];

		inet_ntop(AF_INET, (void *) &multicast_ip, (char *) tmp, 20);
		*errp = pg_error_new("invalid multicast address %s", tmp);
		return;
	}

	/* Allocate a memory buffer to hold an IGMP message */
	pkt[0] = rte_pktmbuf_alloc(mp);
	if (!pkt[0]) {
		*errp = pg_error_new("packet allocation failed");
		return;
	}

	/* Point to the beginning of the IGMP message */
	hdr = (struct multicast_pkt *) rte_pktmbuf_append(pkt[0],
							  IGMP_PKT_LEN);

	ether_addr_copy(&state->mac, &hdr->ethernet.s_addr);
	ether_addr_copy(&dst, &hdr->ethernet.d_addr);
	hdr->ethernet.ether_type = PG_BE_ETHER_TYPE_IPv4;

	/* 4-5 = 0x45 */
	hdr->ipv4.version_ihl = 0x46;
	hdr->ipv4.type_of_service = 0;
	hdr->ipv4.total_length = rte_cpu_to_be_16(sizeof(struct ipv4_hdr) +
						  sizeof(uint32_t) +
						  sizeof(struct igmp_hdr));
	hdr->ipv4.packet_id = 0;
	hdr->ipv4.fragment_offset = 0;
	hdr->ipv4.time_to_live = 1;
	hdr->ipv4.next_proto_id = IGMP_PROTOCOL_NUMBER;
	hdr->ipv4.hdr_checksum = 0;
	hdr->ipv4.src_addr = state->ip;

	hdr->ip_option.type = 0x94;
	hdr->ip_option.size = 4;
	hdr->ip_option.unused = 0;
	switch (action) {
	case IGMP_SUBSCRIBE:
		hdr->ipv4.dst_addr = multicast_ip;
		break;
	case IGMP_UNSUBSCRIBE:
		hdr->ipv4.dst_addr =
			rte_cpu_to_be_32(IPv4(224U, 0, 0, 2));
		break;
	default:
		*errp = pg_error_new("action not handle");
		goto clear;
	}
	hdr->ipv4.hdr_checksum = rte_raw_cksum(&hdr->ipv4,
					       sizeof(struct ipv4_hdr) +
					       sizeof(uint32_t));
	hdr->ipv4.hdr_checksum = ~hdr->ipv4.hdr_checksum;
	hdr->igmp.type = action;
	hdr->igmp.max_resp_time = 0;
	hdr->igmp.checksum = 0;
	hdr->igmp.group_addr = multicast_ip;
	hdr->igmp.checksum = igmp_checksum(&hdr->igmp);

	pg_brick_side_forward(&state->brick.sides[state->output],
			      pg_flip_side(state->output),
			      pkt, pg_mask_firsts(1), errp);

clear:
	rte_pktmbuf_free(pkt[0]);
}

static void multicast4_subscribe(struct vtep_state *state,
				 struct vtep_port *port,
				 uint32_t multicast_ip,
				 struct pg_error **errp)
{
	multicast4_internal(state, port, multicast_ip, IGMP_SUBSCRIBE, errp);
}

static void multicast4_unsubscribe(struct vtep_state *state,
				   struct vtep_port *port,
				   uint32_t multicast_ip,
				   struct pg_error **errp)
{
	multicast4_internal(state, port, multicast_ip, IGMP_UNSUBSCRIBE, errp);
}


int pg_vtep_add_mac(struct pg_brick *brick, uint32_t vni,
		    struct ether_addr *mac, struct pg_error **errp)
{
	if (!strcmp(pg_brick_type(brick), "vtep4"))
		return pg_vtep4_add_mac(brick, vni, mac, errp);
	else
		return pg_vtep6_add_mac(brick, vni, mac, errp);
}

static struct pg_brick_ops vtep_ops = {
	.name		= "vtep4",
	.state_size	= sizeof(struct vtep_state),

	.init		= vtep_init,
	.destroy	= vtep_destroy,

	.unlink		= pg_brick_generic_unlink,

	.unlink_notify  = vtep_unlink_notify,
};


pg_brick_register(vtep, &vtep_ops);

#undef IGMP_PROTOCOL_NUMBER
