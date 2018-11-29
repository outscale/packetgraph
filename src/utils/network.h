/* Copyright 2017 Outscale SAS
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

#ifndef _PG_UTILS_NETWORK_H
#define _PG_UTILS_NETWORK_H

#include <rte_config.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>
#include <rte_ether.h>
#include <endian.h>

#include "utils/bitmask.h"

#define PG_BE_ETHER_TYPE_ARP PG_CPU_TO_BE_16(ETHER_TYPE_ARP)
#define PG_BE_ETHER_TYPE_RARP PG_CPU_TO_BE_16(ETHER_TYPE_RARP)
#define PG_BE_ETHER_TYPE_IPv4 PG_CPU_TO_BE_16(ETHER_TYPE_IPv4)
#define PG_BE_ETHER_TYPE_IPv6 PG_CPU_TO_BE_16(ETHER_TYPE_IPv6)
#define PG_BE_ETHER_TYPE_VLAN PG_CPU_TO_BE_16(ETHER_TYPE_VLAN)
#define PG_BE_IPV4_HDR_DF_FLAG PG_CPU_TO_BE_16(IPV4_HDR_DF_FLAG)
/* 0000 1000 0000 ... */
#define PG_VTEP_I_FLAG 0x08000000
#define PG_VTEP_BE_I_FLAG PG_CPU_TO_BE_32(PG_VTEP_I_FLAG)

#define PG_IP_TYPE_IPV6_OPTION_HOP_BY_HOP 0x00
#define PG_IP_TYPE_IPV6_OPTION_DESTINATION 0x3C
#define PG_IP_TYPE_IPV6_OPTION_ROUTING 0x2B
#define PG_IP_TYPE_IPV6_OPTION_FRAGMENT 0x2C
#define PG_IP_TYPE_IPV6_OPTION_AUTHENTICATION_HEADER 0x33
#define PG_IP_TYPE_IPV6_OPTION_ESP 0x32
#define PG_IP_TYPE_IPV6_OPTION_MOBILITY 0x88
#define PG_IP_TYPE_ICMPV6 0x3A

#define PG_ICMPV6_TYPE_NA 0x88
struct eth_ipv4_hdr {
	struct ether_hdr eth;
	struct ipv4_hdr ip;
} __attribute__((__packed__));

struct eth_ip_l4 {
	struct ether_hdr ethernet;
	union {
		struct {
			struct ipv4_hdr ipv4;
			union {
				struct udp_hdr v4udp;
				struct tcp_hdr v4tcp;
			} __attribute__((__packed__));
		} __attribute__((__packed__));
		struct {
			struct ipv6_hdr ipv6;
			union {
				struct udp_hdr v6udp;
				struct tcp_hdr v6tcp;
			} __attribute__((__packed__));
		} __attribute__((__packed__));
	} __attribute__((__packed__));
} __attribute__((__packed__));

struct vlan_802_1q {
	uint8_t pcp: 3;
	uint8_t cfi: 1;
	uint16_t vid: 12;
	uint16_t ether_type;
} __attribute__((__packed__));

static inline uint16_t pg_utils_get_ether_type(struct rte_mbuf *pkt)
{
	/* Ethertype should be located just before the start of L3.
	 * This way, it works for vlan-tagged and non-vlan tagged
	 * packets (l2_len must be correct).
	 */
	return *(uint16_t *)(rte_pktmbuf_mtod(pkt, uint8_t *) +
			     pkt->l2_len - 2);
}

#define pg_util_get_ether_src_addr(pkt)				\
	(rte_pktmbuf_mtod(pkt, struct ether_addr *) + 1)

#define pg_util_get_ether_dst_addr(pkt)			\
	(rte_pktmbuf_mtod(pkt, struct ether_addr *))


static inline void *pg_utils_get_l3(struct rte_mbuf *pkt)
{
	return rte_pktmbuf_mtod(pkt, char *) + pkt->l2_len;
}

static inline int pg_utils_get_ipv6_l4(struct rte_mbuf *pkt, uint8_t *ip_type,
				       uint8_t **ip_payload)
{
	struct ipv6_hdr *h6 = (struct ipv6_hdr *) pg_utils_get_l3(pkt);
	uint8_t next_header = h6->proto;
	uint8_t *starting_pos = (uint8_t *)(h6 + 1);
	uint8_t *next_data = starting_pos;
	uint8_t loop_cnt = 0;
	bool loop = true;

	while (loop) {
		if (++loop_cnt == 8)
			return -1;
		switch (next_header) {
		case PG_IP_TYPE_IPV6_OPTION_DESTINATION:
		case PG_IP_TYPE_IPV6_OPTION_HOP_BY_HOP:
		case PG_IP_TYPE_IPV6_OPTION_ROUTING:
		case PG_IP_TYPE_IPV6_OPTION_MOBILITY:
			next_header = next_data[0];
			next_data = next_data + (next_data[1] + 1) * 8;
			break;
		case PG_IP_TYPE_IPV6_OPTION_FRAGMENT:
			next_header = next_data[0];
			next_data = next_data + 8;
			break;
		case PG_IP_TYPE_IPV6_OPTION_AUTHENTICATION_HEADER:
			next_header = next_data[0];
			next_data = next_data + (next_data[1] + 2) * 4;
			break;
		case PG_IP_TYPE_IPV6_OPTION_ESP:
			/* cannot get into this */
		default:
			loop = false;
		}
	}
	if (ip_type != NULL) {
		*ip_type = next_header;
		*ip_payload = next_data;
	}
	return next_data - starting_pos;
}

static inline int pg_utils_get_ipv6_l3_len(struct rte_mbuf *pkt)
{
	/* get IPV6 size */
	return pg_utils_get_ipv6_l4(pkt, NULL, NULL)
		+ sizeof(struct ipv6_hdr);
}

static inline void pg_utils_guess_l2(struct rte_mbuf *pkt)
{
	struct ether_hdr *eth;

	pkt->l2_type = RTE_PTYPE_L2_ETHER;
	pkt->l2_len = sizeof(struct ether_hdr);

	eth = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
	if (eth->ether_type == PG_BE_ETHER_TYPE_VLAN) {
		pkt->l2_len += sizeof(struct vlan_802_1q);
		pkt->l2_type |= RTE_PTYPE_L2_ETHER_VLAN;
	}
}

static inline void pg_utils_guess_metadata(struct rte_mbuf *pkt)
{
	pg_utils_guess_l2(pkt);
}

#endif /* ifndef _PG_UTILS_NETWORK_H */
