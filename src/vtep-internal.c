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

_Static_assert(IP_VERSION == 4 || IP_VERSION == 6,
	       "IP_VERSION must be set to 4 or 6");

#define CATCAT(a, b, c) a ## b ## c
#define CAT(a, b) a ## b

#define BRICK_NAME_4 "vtep4"
#define BRICK_NAME_6 "vtep6"

#define BRICK_NAME_(t) CAT(BRICK_NAME_, t)
static const char BRICK_NAME[] = BRICK_NAME_(IP_VERSION);

#define IP_TYPE_4 uint32_t
#define IP_TYPE_6 union pg_ipv6_addr

#define IP_TYPE_(t) CAT(IP_TYPE_, t)
#define IP_TYPE IP_TYPE_(IP_VERSION)

#define IP_IN_TYPE_4 uint32_t
#define IP_IN_TYPE_6 uint8_t *

#define IP_IN_TYPE_(t) CAT(IP_IN_TYPE_, t)
#define IP_IN_TYPE IP_IN_TYPE_(IP_VERSION)

#define ip_hdr_(version) CATCAT(ipv, version, _hdr)
#define ip_hdr ip_hdr_(IP_VERSION)

/**
 * Composite structure of all the headers required to wrap a packet in VTEP
 */
struct headers {
	struct ip_hdr	 ip; /* define in rte_ip.h */
	struct udp_hdr	 udp; /* define in rte_udp.h */
	struct vxlan_hdr vxlan; /* define in rte_ether.h */
} __attribute__((__packed__));

struct full_header {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	struct headers outer;
	struct eth_ip_l4 inner;
} __attribute__((__packed__));

struct dest_addresses {
	struct ether_addr mac;
	IP_TYPE ip;
};

struct vtep_config {
	enum pg_side output;
	struct ether_addr mac;
	IP_TYPE ip; /* IP of the VTEP */
	uint16_t udp_dst_port;
	int flags;
};
			/* 0000 1000 0000 ... */
#define VTEP_I_FLAG	0x08000000

#define UDP_MIN_PORT 49152
#define UDP_PORT_RANGE 16383

#define ip_udptcp_cksum(a, b, version)			\
	CATCAT(rte_ipv, version, _udptcp_cksum)(a, b)

#define HEADER_LENGTH (sizeof(struct headers) + sizeof(struct ether_hdr))

#define ETHER_TYPE_IP_(version) CAT(ETHER_TYPE_IPv, version)
#define ETHER_TYPE_IP ETHER_TYPE_IP_(IP_VERSION)

#define PG_BE_ETHER_TYPE_IP_(version) CAT(PG_BE_ETHER_TYPE_IPv, version)
#define PG_BE_ETHER_TYPE_IP PG_BE_ETHER_TYPE_IP_(IP_VERSION)

#define multicast_subscribe_(ip_version)		\
	CATCAT(multicast, ip_version, _subscribe)
#define multicast_subscribe multicast_subscribe_(IP_VERSION)


#define multicast_unsubscribe_(ip_version)		\
	CATCAT(multicast, ip_version, _unsubscribe)
#define multicast_unsubscribe multicast_unsubscribe_(IP_VERSION)

#define MAC_TO_DST_IS_DEAD 1
#define KNOWN_MAC_IS_DEAD 2
#define HAS_BEEN_BROKEN 4

/* structure used to describe a port of the vtep */
struct vtep_port {
	uint32_t vni;		/* the VNI of this ethernet port */
	IP_TYPE multicast_ip;
	struct pg_mac_table mac_to_dst;
	struct pg_mac_table known_mac;	/* is the MAC adress on this port  */
	int32_t dead_tables;
};

struct vtep_state {
	struct pg_brick brick;
	struct ether_addr mac;		/* MAC address of the VTEP */
	enum pg_side output;		/* side the VTEP packets will go */
	uint16_t udp_dst_port_be;		/* UDP destination port */
	struct vtep_port *ports;
	uint16_t packet_id;       /* IP identification number */
	uint32_t sum;
	int flags;
	struct rte_mbuf *pkts[64];
	IP_TYPE ip; /* IP of the VTEP */
	jmp_buf exeption_env;
	uint64_t out_pkts_mask;
};

static inline int are_mac_tables_dead(struct vtep_port *port)
{
	return port->dead_tables & (MAC_TO_DST_IS_DEAD | KNOWN_MAC_IS_DEAD);
}

static inline void ip6_build(struct vtep_state *state, struct ipv6_hdr *ip_hdr,
			     union pg_ipv6_addr src_ip,
			     union pg_ipv6_addr dst_ip,
			     uint16_t pkt_len)
{
	ip_hdr->vtc_flow = PG_CPU_TO_BE_32(6 << 28);
	ip_hdr->payload_len = rte_cpu_to_be_16(pkt_len);
	ip_hdr->proto = PG_UDP_PROTOCOL_NUMBER;
	ip_hdr->hop_limits = 0xff;

	/* the header checksum computation is to be offloaded in the NIC */
	pg_ip_copy(src_ip, ip_hdr->src_addr);
	pg_ip_copy(dst_ip, ip_hdr->dst_addr);
}

static inline void ip4_build(struct vtep_state *state, struct ipv4_hdr *ip_hdr,
			     uint32_t src_ip, uint32_t dst_ip,
			     uint16_t datagram_len)
{
	uint16_t total_length = rte_cpu_to_be_16(datagram_len);

	ip_hdr->version_ihl = 0x45;
	ip_hdr->type_of_service = 0;
	ip_hdr->total_length = total_length;
	state->packet_id += 1;
	ip_hdr->packet_id = state->packet_id;
	ip_hdr->time_to_live = 64;
	ip_hdr->fragment_offset = 0;
	ip_hdr->next_proto_id = PG_UDP_PROTOCOL_NUMBER;
	ip_hdr->src_addr = src_ip;
	ip_hdr->dst_addr = dst_ip;
	ip_hdr->hdr_checksum = pg_ipv4_udp_cksum(state->sum, total_length,
						 state->packet_id, dst_ip);
}

#define ip_build(state, ip_hdr, src_ip, dst_ip, len)		\
	(_Generic((ip_hdr), struct ipv4_hdr * : ip4_build,	\
		 struct ipv6_hdr * : ip6_build)			\
	 (state, ip_hdr, src_ip, dst_ip, len))

/**
 * Build the VXLAN header
 *
 * @param	header pointer to the VTEP header
 * @param	vni 24 bit Virtual Network Identifier
 */
static inline void vxlan_build(struct vxlan_hdr *header, uint32_t vni)
{
	/* mark the VNI as valid */
	header->vx_flags = PG_VTEP_BE_I_FLAG;

	/**
	 * We have checked the VNI validity at VNI setup so reserved byte will
	 * be zero.
	 */
	header->vx_vni = vni;
}

/**
 * Compute UDP source port to respect good practice from RFC6335:
 * "the Dynamic Ports, also known as the Private or Ephemeral Ports,
 *  from 49152-65535 (never assigned)".
 *
 * @param	seed seed for the src port computation.
 * @return	the resulting UDP source port
 */
static inline uint16_t src_port_compute(uint16_t seed)
{
	_Static_assert((UDP_PORT_RANGE & (UDP_PORT_RANGE + 1)) == 0,
		       "value must be power of 2 minus 1");
	return (seed & UDP_PORT_RANGE) + UDP_MIN_PORT;
}

/**
 * Build the UDP header
 *
 * @param	ip pointer to the IP header
 * @param	udp_hdr pointer to the UDP header
 * @param	udp_dst_port UDP destination port
 * @param	datagram_len length of the UDP datagram
 * @param	seed seed for udp src port building
 */
static inline void udp_build_cksum(struct ip_hdr *ip,
				   struct udp_hdr *udp_hdr,
				   uint16_t udp_dst_port,
				   uint16_t datagram_len,
				   uint16_t seed)
{
	udp_hdr->src_port = rte_cpu_to_be_16(src_port_compute(seed));
	udp_hdr->dst_port = udp_dst_port;
	udp_hdr->dgram_len = rte_cpu_to_be_16(datagram_len);
	/* UDP checksum SHOULD be transmited as zero */
	udp_hdr->dgram_cksum = 0;
#if IP_VERSION == 6
	udp_hdr->dgram_cksum = ip_udptcp_cksum(ip, udp_hdr, 6);
#endif
}

static inline void udp_build(struct udp_hdr *udp_hdr,
			     uint16_t udp_dst_port,
			     uint16_t datagram_len,
			     uint16_t seed)
{
	udp_hdr->src_port = rte_cpu_to_be_16(src_port_compute(seed));
	udp_hdr->dst_port = udp_dst_port;
	udp_hdr->dgram_len = rte_cpu_to_be_16(datagram_len);
	/* UDP checksum SHOULD be transmited as zero */
	udp_hdr->dgram_cksum = 0;
}

/**
 * Build the ethernet header
 *
 * @param	eth_hdr pointer to the ethernet header
 * @param	src_mac source MAC address
 * @param	dst_mac destination MAC address
 */
static inline void ethernet_build(struct ether_hdr *eth_hdr,
				  struct ether_addr *src_mac,
				  struct ether_addr *dst_mac)
{
	/* copy mac addresses */
	ether_addr_copy(src_mac, &eth_hdr->s_addr);
	ether_addr_copy(dst_mac, &eth_hdr->d_addr);

	/* the ethernet frame carries an IP packet */
	eth_hdr->ether_type = PG_BE_ETHER_TYPE_IP;
}

static inline uint16_t udp_overhead(void)
{
	return sizeof(struct vxlan_hdr) + sizeof(struct udp_hdr);
}

static inline uint16_t ip_overhead(void)
{
	return udp_overhead() + sizeof(struct ipv4_hdr);
}


static inline void do_add_mac(struct vtep_port *port, struct ether_addr *mac);

static inline int vtep_header_prepend(struct vtep_state *state,
				      struct rte_mbuf *pkt,
				      struct vtep_port *port,
				      struct dest_addresses *entry,
				      int unicast,
				      struct pg_error **errp)
{
	struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
	uint16_t packet_len = rte_pktmbuf_data_len(pkt);
	struct full_header *full_header;
	struct headers *headers;
	struct ether_hdr *ethernet;

	full_header =
		(struct full_header *)rte_pktmbuf_prepend(pkt, HEADER_LENGTH);
	if (unlikely(!full_header)) {
		*errp = pg_error_new("%s on a packet of %ld/%d",
				     "No enough headroom to add VTEP headers",
				     HEADER_LENGTH, pkt->pkt_len);
		return -1;
	}

	ethernet = &full_header->ethernet;
	headers = &full_header->outer;
	vxlan_build(&headers->vxlan, port->vni);

	/* select destination IP and MAC address */
	if (likely(unicast)) {
		ethernet_build(ethernet, &state->mac, &entry->mac);
		ip_build(state, &headers->ip, state->ip, entry->ip,
			 packet_len + ip_overhead());
	} else {
		struct ether_addr dst =
			pg_multicast_get_dst_addr(port->multicast_ip);

		if (!(state->flags & PG_VTEP_NO_INNERMAC_CHECK))
			do_add_mac(port, &eth_hdr->s_addr);
		ethernet_build(ethernet, &state->mac, &dst);
		ip_build(state, &headers->ip, state->ip, port->multicast_ip,
			 packet_len + ip_overhead());
	}
	/*
	 * It is recommended to have UDP source port randomized to be
	 * ECMP/load-balancing friendly. Let's use computed hash from
	 * IP header.
	 */
	if (unlikely(state->flags & PG_VTEP_FORCE_UPD_IPV6_CHECKSUM)) {
		udp_build_cksum(&headers->ip, &headers->udp,
				state->udp_dst_port_be,
				packet_len + udp_overhead(),
				((uint16_t *)pkt)[0]);
	} else {
		udp_build(&headers->udp, state->udp_dst_port_be,
			  packet_len + udp_overhead(), ((uint16_t *)pkt)[0]);
	}

	pkt->l2_len = HEADER_LENGTH + sizeof(struct ether_hdr);

	if (unlikely(pkt->udata64 & PG_FRAGMENTED_MBUF)) {
		pkt->l2_len = sizeof(struct ether_hdr);
		pkt->l3_len = sizeof(struct ipv4_hdr);
		pkt->ol_flags |= PKT_TX_UDP_CKSUM;
	} else if (pkt->ol_flags & PKT_TX_TCP_SEG) {
		pkt->ol_flags |= PKT_TX_IPV4 | PKT_TX_IP_CKSUM |
			PKT_TX_TCP_CKSUM | PKT_TX_TCP_SEG;
	}

	return 0;
}

static inline int vtep_encapsulate(struct vtep_state *state,
				   struct vtep_port *port,
				   struct rte_mbuf **pkts, uint64_t pkts_mask,
				   struct pg_error **errp)
{
	struct rte_mempool *mp = pg_get_mempool();

	/* do the encapsulation */
	for (; pkts_mask;) {
		struct dest_addresses *entry = NULL;
		struct rte_mbuf *pkt;
		int unicast;
		uint16_t i;
		struct rte_mbuf *tmp;

		pg_low_bit_iterate(pkts_mask, i);

		pkt = pkts[i];
		/* must we encapsulate in an unicast VTEP header */
		unicast = !is_multicast_ether_addr(dst_key_ptr(pkt));

		/* pick up the right destination ip */
		if (likely(unicast)) {
			struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(
				pkt,
				struct ether_hdr *);

			entry = pg_mac_table_elem_get(
				&port->mac_to_dst,
				*((union pg_mac *)&eth_hdr->d_addr),
				struct dest_addresses);
			if (unlikely(!entry))
				unicast = 0;
		}

		if (unlikely(!(state->flags & PG_VTEP_NO_COPY))) {
			tmp = rte_pktmbuf_clone(pkt, mp);
			if (unlikely(!tmp))
				return -1;

		} else {
			tmp = pkt;
		}

		if (unlikely(vtep_header_prepend(state, tmp, port,
						  entry, unicast, errp)) < 0) {
			if (!(state->flags & PG_VTEP_NO_COPY))
				rte_pktmbuf_free(tmp);
			return -1;
		}
		state->pkts[i] = tmp;

	}

	return 0;
}

static inline int try_fix_tables(struct vtep_state *state,
				 struct vtep_port *port,
				 struct pg_error **errp)
{
	if (port->dead_tables & MAC_TO_DST_IS_DEAD) {
		if (pg_mac_table_init(&port->mac_to_dst,
				      &state->exeption_env) < 0) {
			*errp = pg_error_new_errno(ENOMEM, "out of memory");
			return -1;
		}
		port->dead_tables ^= MAC_TO_DST_IS_DEAD;
	}

	if (port->dead_tables & KNOWN_MAC_IS_DEAD) {
		if (pg_mac_table_init(&port->known_mac,
				      &state->exeption_env)  < 0) {
			*errp = pg_error_new_errno(ENOMEM, "out of memory");
			return -1;
		}
		port->dead_tables ^= KNOWN_MAC_IS_DEAD;
	}
	return 0;
}

static inline int to_vtep(struct pg_brick *brick, enum pg_side from,
		    uint16_t edge_index, struct rte_mbuf **pkts,
		    uint64_t pkts_mask,
		    struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct vtep_port *port = &state->ports[edge_index];
	int ret;

	if (unlikely(!(state->flags & PG_VTEP_NO_INNERMAC_CHECK))) {
		if (setjmp(state->exeption_env)) {
			port->dead_tables = KNOWN_MAC_IS_DEAD | HAS_BEEN_BROKEN;
			pg_mac_table_free(&port->known_mac);
			if (!state->flags & PG_VTEP_NO_COPY)
				pg_packets_free(state->pkts,
						state->out_pkts_mask);
			*errp = pg_error_new_errno(ENOMEM,
						   "not enouth space to add mac");
			return -1;
		}
	}

	if (unlikely(are_mac_tables_dead(port) &&
		     try_fix_tables(state, port, errp) < 0))
		return -1;
	/* if the port VNI is not set up ignore the packets */
	if (unlikely(!pg_is_multicast_ip(port->multicast_ip)))
		return 0;

	if (unlikely(vtep_encapsulate(state, port, pkts, pkts_mask, errp) < 0))
		return -1;

	ret = pg_brick_side_forward(s, from, state->pkts, pkts_mask, errp);
	if (!(state->flags & PG_VTEP_NO_COPY))
		pg_packets_free(state->pkts, pkts_mask);
	return ret;
}

static inline void add_dst_iner_macs(struct vtep_state *state,
				     struct vtep_port *port,
				     struct rte_mbuf **pkts,
				     struct ether_addr **eths,
				     struct headers **hdrs,
				     uint64_t pkts_mask,
				     uint64_t multicast_mask)
{
	uint64_t mask;
	uint64_t bit;
	union pg_mac tmp;

	tmp.mac = 0;

	if (unlikely(port->dead_tables))
		multicast_mask = pkts_mask;

	for (mask = multicast_mask; mask;) {
		int i;
		struct dest_addresses dst;
		struct ether_hdr *pkt_addr;

		pg_low_bit_iterate_full(mask, bit, i);

		pkt_addr = rte_pktmbuf_mtod(pkts[i], struct ether_hdr *);
		ether_addr_copy(eths[i], &dst.mac);
#if IP_VERSION == 4
		dst.ip = hdrs[i]->ip.src_addr;
#else
		pg_ip_copy(hdrs[i]->ip.src_addr, &dst.ip);
#endif
		rte_memcpy(tmp.bytes, &pkt_addr->s_addr.addr_bytes, 6);
		pg_mac_table_elem_set(&port->mac_to_dst, tmp, &dst,
				      sizeof(struct dest_addresses));
	}
}

/**
 * @return false if checksum is not valid
 */
static inline bool check_udp_checksum(struct headers *hdr)
{
	uint16_t cksum = hdr->udp.dgram_cksum;

#if IP_VERSION == 4
	hdr->ip.hdr_checksum = 0;
#endif
	hdr->udp.dgram_cksum = 0;
	return (ip_udptcp_cksum(&hdr->ip, &hdr->udp, IP_VERSION) == cksum);
}


/**
 * @multicast_mask store multicast packets
 * @computed_mask store package contening a valide vxlan header
 */
static inline void classify_pkts(struct rte_mbuf **pkts,
				 uint64_t mask,
				 struct ether_addr **eths,
				 struct headers **hdrs,
				 uint64_t *multicast_mask,
				 uint64_t *computed_mask,
				 uint16_t udp_dst_port_be)
{
	for (*multicast_mask = 0, *computed_mask = 0; mask;) {
		int i;
		struct headers *tmp;

		pg_low_bit_iterate(mask, i);
		tmp = pg_utils_get_l3(pkts[i]);
		eths[i] = pg_util_get_ether_src_addr(pkts[i]);
		hdrs[i] = tmp;
		if (unlikely(pg_ip_proto(tmp->ip) != 17 ||
			     tmp->udp.dst_port != udp_dst_port_be ||
			     tmp->vxlan.vx_flags != PG_VTEP_BE_I_FLAG))
			continue;
		if (tmp->udp.dgram_cksum) {
			if (unlikely(!check_udp_checksum(hdrs[i])))
				continue;
			pkts[i]->ol_flags |= PKT_TX_UDP_CKSUM;
			pkts[i]->ol_flags |= PKT_TX_TCP_CKSUM;
		}
		if (pg_is_multicast_ip(tmp->ip.dst_addr))
			*multicast_mask |= (1LLU << i);
		*computed_mask |= (1LLU << i);
	}
}

static inline uint64_t check_and_clone_vni_pkts(struct vtep_state *state,
						struct rte_mbuf **pkts,
						uint64_t mask,
						struct headers **hdrs,
						struct vtep_port *port,
						struct rte_mbuf **out_pkts)
{
	uint64_t vni_mask = 0;

	for (; mask;) {
		int j;

		pg_low_bit_iterate(mask, j);
		if (hdrs[j]->vxlan.vx_vni == port->vni) {
			struct rte_mbuf *tmp;

			if (unlikely(!(state->flags & PG_VTEP_NO_COPY))) {
				struct rte_mempool *mp = pg_get_mempool();

				tmp = rte_pktmbuf_clone(pkts[j], mp);
			} else {
				tmp = pkts[j];
			}
			if (unlikely(!tmp))
				return 0;
			out_pkts[j] = tmp;
			if (!rte_pktmbuf_adj(out_pkts[j],
					     out_pkts[j]->l2_len +
					     sizeof(struct headers)))
				return 0;
			vni_mask |= (1LLU << j);
		}
	}
	return vni_mask;
}

static inline void restore_metadata(struct rte_mbuf **pkts,
				    struct headers **hdrs,
				    uint64_t vni_mask)
{
	PG_FOREACH_BIT(vni_mask, it) {
		pkts[it]->l2_len = sizeof(struct ether_hdr);
		uint16_t eth_type = pg_utils_get_ether_type(pkts[it]);

		switch (eth_type) {
		case PG_BE_ETHER_TYPE_IPv4:
			pkts[it]->l3_len = sizeof(struct ipv4_hdr);
			break;
		case PG_BE_ETHER_TYPE_IPv6:
			pkts[it]->l3_len = sizeof(struct ipv6_hdr);
			break;
		}
	}
}

static inline int decapsulate(struct pg_brick *brick, enum pg_side from,
			      uint16_t edge_index, struct rte_mbuf **pkts,
			      uint64_t pkts_mask,
			      struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct vtep_port *ports = state->ports;
	struct ether_addr *eths[64];
	struct headers *hdrs[64];
	struct rte_mbuf **out_pkts = state->pkts;
	uint64_t multicast_mask;

	classify_pkts(pkts, pkts_mask, eths, hdrs, &multicast_mask, &pkts_mask,
		      state->udp_dst_port_be);

	for (int i = 0; i < s->nb; ++i) {
		struct vtep_port *port = &ports[i];
		uint64_t hitted_mask = 0;
		uint64_t vni_mask;

		if (!pkts_mask)
			break;
		if (unlikely(are_mac_tables_dead(port) &&
			     try_fix_tables(state, port,errp) < 0))
			return -1;
		/* Decaspulate and check the vni*/
		vni_mask = check_and_clone_vni_pkts(state, pkts, pkts_mask,
						    hdrs, port, out_pkts);
		if (!vni_mask)
			continue;

		pkts_mask ^= vni_mask;
		if (state->flags & PG_VTEP_NO_INNERMAC_CHECK) {
			hitted_mask = vni_mask;
		} else {
			PG_FOREACH_BIT(vni_mask, it) {
				void *entry;
				struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(
					out_pkts[it],
					struct ether_hdr *);

				entry = pg_mac_table_ptr_get(
					&port->known_mac,
					*((union pg_mac *)&eth_hdr->d_addr));

				if (entry)
					hitted_mask |= ONE64 << it;
			}
		}

		if (hitted_mask) {
			add_dst_iner_macs(state, port, out_pkts, eths, hdrs,
					  hitted_mask, multicast_mask);

			restore_metadata(out_pkts, hdrs, hitted_mask);
			if (unlikely(pg_brick_burst(s->edges[i].link, from,
						    i, out_pkts, hitted_mask,
						    errp) < 0)) {
				if (!(state->flags & PG_VTEP_NO_COPY))
					pg_packets_free(out_pkts, vni_mask);
				return -1;
			}
		}

		if (unlikely(!(state->flags & PG_VTEP_NO_COPY)))
			pg_packets_free(out_pkts, vni_mask);
	}
	return 0;
}

static inline uint64_t check_vni_pkts(struct rte_mbuf **pkts,
				      uint64_t mask,
				      struct headers **hdrs,
				      struct vtep_port *port)
{
	uint64_t vni_mask = 0;

	for (; mask;) {
		int j;
		uint64_t bit;

		pg_low_bit_iterate_full(mask, bit, j);
		if (hdrs[j]->vxlan.vx_vni == port->vni) {
			rte_pktmbuf_adj(pkts[j],
					pkts[j]->l2_len +
					sizeof(struct headers));
			vni_mask |= bit;
		}
	}
	return vni_mask;
}

static inline int decapsulate_simple(struct pg_brick *brick, enum pg_side from,
				     uint16_t edge_index,
				     struct rte_mbuf **pkts,
				     uint64_t pkts_mask,
				     struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct vtep_port *ports = state->ports;
	struct ether_addr *eths[64];
	struct headers *hdrs[64];
	struct pg_brick_edge *edges = s->edges;
	uint64_t multicast_mask;

	classify_pkts(pkts, pkts_mask, eths, hdrs, &multicast_mask, &pkts_mask,
		      state->udp_dst_port_be);

	for (int i = 0, nb = s->nb; pkts_mask && i < nb; ++i) {
		struct vtep_port *port = &ports[i];
		uint64_t vni_mask;

		if (unlikely(are_mac_tables_dead(port) &&
			     try_fix_tables(state, port, errp) < 0))
			return -1;
		/* Decaspulate and check the vni*/
		vni_mask = check_vni_pkts(pkts, pkts_mask,
					  hdrs, port);
		if (!vni_mask)
			continue;

		pkts_mask ^= vni_mask;
		add_dst_iner_macs(state, port, pkts, eths, hdrs,
				  vni_mask, multicast_mask);
		restore_metadata(pkts, hdrs, vni_mask);

		if (unlikely(pg_brick_burst(edges[i].link, from, i, pkts,
					    vni_mask, errp) < 0))
			return -1;
	}
	return 0;
}

static inline int from_vtep(struct pg_brick *brick, enum pg_side from,
		      uint16_t edge_index, struct rte_mbuf **pkts,
		      uint64_t pkts_mask,
		      struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);


	if (unlikely(setjmp(state->exeption_env))) {
		struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];

		/* free all ports */
		for (int i = 0; i < s->nb; ++i) {
			struct vtep_port *port = &state->ports[i];

			port->dead_tables = MAC_TO_DST_IS_DEAD |
				HAS_BEEN_BROKEN;
			pg_mac_table_free(&port->mac_to_dst);
		}
		*errp = pg_error_new_errno(ENOMEM,
					   "not enouth space to add mac");
		return -1;
	}

	if (state->flags == PG_VTEP_ALL_OPTI)
		return decapsulate_simple(brick, from, edge_index, pkts,
					  pkts_mask, errp);
	else
		return decapsulate(brick, from, edge_index, pkts,
				   pkts_mask, errp);
}

static int do_add_vni(struct vtep_state *state, uint16_t edge_index,
		       int32_t vni, IP_TYPE multicast_ip,
		       struct pg_error **errp)
{
	struct vtep_port *port = &state->ports[edge_index];

	if (unlikely(!port)) {
		*errp = pg_error_new("bad vtep internal port provided");
		return -1;
	}
	if (unlikely(port->vni)) {
		*errp = pg_error_new("port already attached to a vni");
		return -1;
	}
	if (unlikely(pg_is_multicast_ip(port->multicast_ip))) {
		*errp = pg_error_new("port alread have a mutlicast IP");
		return -1;
	}
	/* vni is on the first 24 bits */
	port->vni = rte_cpu_to_be_32(vni << 8);
	pg_ip_copy(multicast_ip, &port->multicast_ip);

	g_assert(!pg_mac_table_init(&port->mac_to_dst, &state->exeption_env));
	g_assert(!pg_mac_table_init(&port->known_mac, &state->exeption_env));

	multicast_subscribe(state, port, multicast_ip, errp);
	return 0;
}

static inline void do_add_mac(struct vtep_port *port, struct ether_addr *mac)
{
	union pg_mac tmp;

	tmp.mac = 0;
	rte_memcpy(tmp.bytes, mac->addr_bytes, 6);
	pg_mac_table_ptr_set(&port->known_mac, tmp, (void *)1);
}

static void do_remove_vni(struct vtep_state *state,
		   uint16_t edge_index, struct pg_error **errp)
{
	struct vtep_port *port = &state->ports[edge_index];

	if (!pg_is_multicast_ip(port->multicast_ip))
		return;

	multicast_unsubscribe(state, port, port->multicast_ip,
			      errp);

	if (pg_error_is_set(errp))
		return;

	/* Do the hash destroy at the end since it's the less idempotent */
	if (!(port->dead_tables & KNOWN_MAC_IS_DEAD))
		pg_mac_table_free(&port->known_mac);
	if (!(port->dead_tables & MAC_TO_DST_IS_DEAD))
		pg_mac_table_free(&port->mac_to_dst);

	/* clear for next user */
	memset(port, 0, sizeof(struct vtep_port));
}

/**
 * Is the given VNI valid
 *
 * @param	vni the VNI to check
 * @return	1 if true, 0 if false
 */
static bool is_vni_valid(uint32_t vni)
{
	/* VNI is coded on 24 bits, must be < 2^24 */
	return vni < 16777216;
}

static void vtep_unlink_notify(struct pg_brick *brick,
				enum pg_side side, uint16_t edge_index,
				struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);

	if (side == state->output)
		return;

	do_remove_vni(state, edge_index, errp);
}

static void vtep_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);

	g_free(state->ports);
}

static struct pg_brick_config *vtep_config_new(const char *name,
					       uint32_t west_max,
					       uint32_t east_max,
					       enum pg_side output,
					       IP_TYPE ip,
					       struct ether_addr mac,
					       uint16_t udp_dst_port,
					       int flags)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct vtep_config *vtep_config = g_new0(struct vtep_config, 1);

	vtep_config->output = output;
	pg_ip_copy(ip, &vtep_config->ip);
	vtep_config->flags = flags;
	vtep_config->udp_dst_port = udp_dst_port;
	ether_addr_copy(&mac, &vtep_config->mac);
	config->brick_config = vtep_config;
	return pg_brick_config_init(config, name, west_max,
				    east_max, PG_MULTIPOLE);
}

static int vtep_burst(struct pg_brick *brick, enum pg_side from,
			uint16_t edge_index, struct rte_mbuf **pkts,
			uint64_t pkts_mask,
			struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);

	/*
	 * if pkts come from the outside,
	 * so the pkts are entering in the vtep
	 */
	if (from == state->output)
		return from_vtep(brick, from, edge_index,
				 pkts, pkts_mask, errp);
	else
		return to_vtep(brick, from, edge_index,
			       pkts, pkts_mask, errp);
}


static int vtep_init(struct pg_brick *brick,
		     struct pg_brick_config *config, struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	struct vtep_config *vtep_config;
	uint16_t max;

	vtep_config = config->brick_config;
	state->output = vtep_config->output;
	state->ip = vtep_config->ip;
	ether_addr_copy(&vtep_config->mac, &state->mac);
	state->flags = vtep_config->flags;
	state->udp_dst_port_be = rte_cpu_to_be_16(vtep_config->udp_dst_port);
	#if IP_VERSION == 4
	state->sum = pg_ipv4_init_cksum(state->ip);
	state->packet_id = 0;
	#endif

	/*
	 * do a lazy allocation of the VTEP ports: the code will init them
	 * at VNI port add
	 */
	max = pg_side_get_max(brick, pg_flip_side(state->output));
	state->ports = g_new0(struct vtep_port, max);

	brick->burst = vtep_burst;
	return 0;
}

#define pg_vtep_add_mac__(version) CATCAT(pg_vtep, version, _add_mac)
#define pg_vtep_add_mac_ pg_vtep_add_mac__(IP_VERSION)

int pg_vtep_add_mac_(struct pg_brick *brick, uint32_t vni,
		     struct ether_addr *mac, struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(state->output)];

	vni = rte_cpu_to_be_32(vni << 8);
	for (int i = 0; i < s->nb; ++i) {
		if (state->ports[i].vni == vni) {
			struct vtep_port *port = &state->ports[i];

			if (unlikely(setjmp(state->exeption_env))) {
				port->dead_tables = KNOWN_MAC_IS_DEAD;
				pg_mac_table_free(&port->mac_to_dst);
				*errp = pg_error_new_errno(ENOMEM,
							   "not enouth space to add mac");
				return -1;
			}
			do_add_mac(port, mac);
			return 0;
		}
	}
	*errp = pg_error_new("vni '%d' not found in brick '%s'", vni,
			     brick->name);
	return -1;
}


#define pg_vtep_new__(version) CATCAT(pg_vtep, version, _new)
#define pg_vtep_new_ pg_vtep_new__(IP_VERSION)

struct pg_brick *pg_vtep_new_(const char *name, uint32_t max,
			      enum pg_side output, IP_IN_TYPE ip,
			      struct ether_addr mac,
			      uint16_t udp_dst_port, int flags,
			      struct pg_error **errp)
{
	uint32_t west_max;
	uint32_t east_max;
	IP_TYPE tmp_ip;
	struct pg_brick_config *config;
	struct pg_brick *ret;

	if (output == PG_WEST_SIDE) {
		west_max = 1;
		east_max = max;
	} else {
		west_max = max;
		east_max = 1;
	}
	pg_ip_copy(ip, &tmp_ip);
	config = vtep_config_new(name, west_max,
				 east_max, output,
				 tmp_ip, mac,
				 udp_dst_port, flags);
	ret = pg_brick_new(BRICK_NAME, config, errp);

	pg_brick_config_free(config);
	return ret;
}

#define pg_vtep_add_vni__(version) CATCAT(pg_vtep, version, _add_vni)
#define pg_vtep_add_vni_ pg_vtep_add_vni__(IP_VERSION)

int pg_vtep_add_vni_(struct pg_brick *brick,
		     struct pg_brick *neighbor,
		     uint32_t vni, IP_IN_TYPE multicast_ip,
		     struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	enum pg_side side = pg_flip_side(state->output);
	uint16_t i;
	int found;
	IP_TYPE tmp_ip;
	struct ether_addr mac = {{0xff, 0xff, 0xff,
				  0xff, 0xff, 0xff} };

	if (!brick) {
		*errp = pg_error_new("brick is NULL");
		return -1;
	}

	if (!neighbor) {
		*errp = pg_error_new("VTEP brick is NULL");
		return -1;
	}

	if (!is_vni_valid(vni)) {
		*errp = pg_error_new("Invalid VNI");
		return -1;
	}

	pg_ip_copy(multicast_ip, &tmp_ip);

	if (!pg_is_multicast_ip(tmp_ip)) {
		*errp = pg_error_new(
			"Provided IP is not in the multicast range");
		return -1;
	}

	/* lookup for the vtep brick index */
	found = 0;
	for (i = 0; i < brick->sides[side].max; i++)
		if (neighbor == brick->sides[side].edges[i].link) {
			found = 1;
			break;
		}

	if (!found) {
		*errp = pg_error_new("VTEP brick index not found");
		return -1;
	}

	if (do_add_vni(state, i, vni, tmp_ip, errp) < 0)
		return -1;
	do_add_mac(&state->ports[i], &mac);
	return 0;
}

#undef VTEP_I_FLAG
#undef UDP_MIN_PORT
#undef UDP_PORT_RANGE
#undef CATCAT
#undef CAT
#undef ip_udptcp_cksum
#undef header_
#undef header
#undef fullhdr_
#undef fullhdr
#undef HEADER_LENGTH
#undef ETHER_TYPE_IP_
#undef ETHER_TYPE_IP
#undef PG_BE_ETHER_TYPE_IP_
#undef PG_BE_ETHER_TYPE_IP
#undef multicast_subscribe_
#undef multicast_subscribe
#undef multicast_unsubscribe_
#undef multicast_unsubscribe
#undef ip_build
#undef BRICK_NAME_4
#undef BRICK_NAME_6
#undef BRICK_NAME_
#undef IP_TYPE_4
#undef IP_TYPE_6
#undef IP_TYPE_
#undef IP_TYPE
#undef IP_IN_TYPE_4
#undef IP_IN_TYPE_6
#undef IP_IN_TYPE_
#undef IP_IN_TYPE
