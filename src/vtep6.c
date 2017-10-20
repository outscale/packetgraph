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
#include <packetgraph/errors.h>
#include <packetgraph/vtep.h>
#include "brick-int.h"
#include "packets.h"
#include "utils/bitmask.h"
#include "utils/mempool.h"
#include "utils/mac-table.h"
#include "utils/mac.h"
#include "utils/ip.h"
#include "utils/network.h"

#include "vtep-internal.h"

static void multicast6_subscribe(struct vtep_state *state,
				 struct vtep_port *port,
				 union pg_ipv6_addr multicast_ip,
				 struct pg_error **errp);

static void multicast6_unsubscribe(struct vtep_state *state,
				   struct vtep_port *port,
				   union pg_ipv6_addr multicast_ip,
				   struct pg_error **errp);

#define IP_VERSION 6
#include "vtep-internal.c"
#undef IP_VERSION

struct mld_hdr {
	uint8_t type;
	uint8_t code;
	uint16_t cksum;
	uint32_t max_response_time;
	uint32_t reserved;
	union pg_ipv6_addr multicast_addr;
} __attribute__((__packed__));

struct multicast_pkt {
	struct ether_hdr ethernet;
	struct ipv6_hdr ipv6;
	struct mld_hdr mld;
} __attribute__((__packed__));

static void  multicast6_internal(struct vtep_state *state,
				struct vtep_port *port,
				union pg_ipv6_addr *multicast_ip,
				enum operation action,
				struct pg_error **errp);


static void multicast6_subscribe(struct vtep_state *state,
				struct vtep_port *port,
				union pg_ipv6_addr multicast_ip,
				struct pg_error **errp)
{
	multicast6_internal(state, port, &multicast_ip, MLD_SUBSCRIBE,
			    errp);
}

static void multicast6_unsubscribe(struct vtep_state *state,
				  struct vtep_port *port,
				  union pg_ipv6_addr multicast_ip,
				  struct pg_error **errp)
{
	multicast6_internal(state, port, &multicast_ip, MLD_UNSUBSCRIBE,
			    errp);
}


static void multicast6_internal(struct vtep_state *state,
				struct vtep_port *port,
				union pg_ipv6_addr *multicast_ip,
				enum operation action,
				struct pg_error **errp)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt[1];
	struct multicast_pkt *hdr;
	struct ether_addr dst = pg_multicast_get_dst_addr(*multicast_ip);

	if (!pg_is_multicast_ip(multicast_ip)) {
		*errp = pg_error_new("invalid multicast address %s",
				     pg_ipv6_to_str(multicast_ip->word8));
		return;
	}

	pkt[0] = rte_pktmbuf_alloc(mp);
	if (!pkt[0]) {
		*errp = pg_error_new("packet allocation failed");
		return;
	}
	hdr = (void *)rte_pktmbuf_append(pkt[0],
					 sizeof(struct multicast_pkt));

	ether_addr_copy(&state->mac, &hdr->ethernet.s_addr);
	ether_addr_copy(&dst, &hdr->ethernet.d_addr);
	hdr->ethernet.ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv6);

	hdr->ipv6.vtc_flow = PG_CPU_TO_BE_32(6 << 28);
	hdr->ipv6.payload_len = rte_cpu_to_be_16(sizeof(struct multicast_pkt) -
		sizeof(struct ether_hdr));
	#define ICMPV6_PROTOCOL_NUMBER 58
	hdr->ipv6.proto = ICMPV6_PROTOCOL_NUMBER;
	hdr->ipv6.hop_limits = 0xff;

	/* the header checksum computation is to be offloaded in the NIC */
	pg_ip_copy(&state->ip, hdr->ipv6.src_addr);
	switch (action) {
	case MLD_SUBSCRIBE:
		pg_ip_from_str(hdr->ipv6.dst_addr, "ff02::2");
		break;
	case MLD_UNSUBSCRIBE:
		pg_ip_from_str(hdr->ipv6.dst_addr, "ff02::1");
		break;
	default:
		*errp = pg_error_new("action not handle");
		goto clear;
	}
	hdr->mld.type = action;
	hdr->mld.code = 0;
	hdr->mld.cksum = 0;
	hdr->mld.max_response_time = 0;
	hdr->mld.reserved = 0;
	pg_ip_copy(multicast_ip, &hdr->mld.multicast_addr);
	pg_brick_side_forward(&state->brick.sides[state->output],
			      pg_flip_side(state->output),
			      pkt, 1, errp);

clear:
	rte_pktmbuf_free(pkt[0]);
}

static struct pg_brick_ops vtep_ops = {
	.name		= BRICK_NAME,
	.state_size	= sizeof(struct vtep_state),

	.init		= vtep_init,
	.destroy	= vtep_destroy,

	.unlink		= pg_brick_generic_unlink,

	.unlink_notify  = vtep_unlink_notify,
};

pg_brick_register(vtep, &vtep_ops);

