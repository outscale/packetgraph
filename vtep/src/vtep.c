/* Copyright 2015 Nodalink EURL
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
#include <rte_hash_crc.h>
#include <rte_ip.h>
#include <rte_memcpy.h>
#include <rte_table.h>
#include <rte_table_hash.h>
#include <rte_prefetch.h>
#include <rte_udp.h>

#include <packetgraph/brick.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/common.h>
#include <packetgraph/packets.h>
#include <packetgraph/vtep.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/mac-table.h>
#include <packetgraph/utils/mac.h>

struct dest_addresses {
	uint32_t ip;
	struct ether_addr mac;
};

struct vtep_config {
	enum pg_side output;
	int32_t ip;
	struct ether_addr mac;
	int flags;
};

			/* 0000 1000 0000 ... */
#define VTEP_I_FLAG	0x08000000
#define VTEP_DST_PORT	4789

#define UDP_MIN_PORT 49152
#define UDP_PORT_RANGE 16383
#define UDP_PROTOCOL_NUMBER 17
#define IGMP_PROTOCOL_NUMBER 0x02
/**
 * Composite structure of all the headers required to wrap a packet in VTEP
 */
struct headers {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	struct ipv4_hdr	 ipv4; /* define in rte_ip.h */
	struct udp_hdr	 udp; /* define in rte_udp.h */
	struct vxlan_hdr vxlan; /* define in rte_ether.h */
} __attribute__((__packed__));

struct igmp_hdr {
	uint8_t type;
	uint8_t max_resp_time;
	uint16_t checksum;
	uint32_t group_addr;
} __attribute__((__packed__));

struct multicast_pkt {
	struct ether_hdr ethernet;
	struct ipv4_hdr ipv4;
	struct igmp_hdr igmp;
} __attribute__((__packed__));


#define HEADERS_LENGTH sizeof(struct headers)
#define IGMP_PKT_LEN sizeof(struct multicast_pkt)

/* structure used to describe a port of the vtep */
struct vtep_port {
	uint32_t vni;		/* the VNI of this ethernet port */
	uint32_t multicast_ip;  /* the multicast ip associated with the VNI */
	struct pg_mac_table mac_to_dst;
	struct pg_mac_table known_mac;	/* is the MAC adress on this port  */
};

enum operation {
	IGMP_SUBSCRIBE = 0x16,
	IGMP_UNSUBSCRIBE = 0x17,
};

struct vtep_state {
	struct pg_brick brick;
	uint32_t ip;			/* IP of the VTEP */
	uint64_t *masks;		/* internal port packet masks */
	enum pg_side output;		/* the side the VTEP packets will go */
	uint16_t dst_port;		/* the UDP destination port */
	struct ether_addr mac;		/* MAC address of the VTEP */
	struct vtep_port *ports;
	rte_atomic16_t packet_id;	/* IP identification number */
	int flags;
	struct rte_mbuf *pkts[64];
};

inline struct ether_addr *pg_vtep_get_mac(struct pg_brick *brick)
{
	return brick ? &pg_brick_get_state(brick, struct vtep_state)->mac :
		NULL;
}

static inline void do_add_mac(struct vtep_port *port, struct ether_addr *mac);
static void  multicast_internal(struct vtep_state *state,
				struct vtep_port *port,
				uint32_t multicast_ip,
				struct pg_error **errp,
				enum operation action);
static inline uint64_t hash_32(void *key, uint32_t key_size, uint64_t seed)
{
	return _mm_crc32_u32(seed, *((uint64_t *) key));
}

static inline uint64_t hash_64(void *key, uint32_t key_size, uint64_t seed)
{
	return _mm_crc32_u64(seed, *((uint64_t *) key));
}


static inline struct ether_addr multicast_get_dst_addr(uint32_t ip)
{
	struct ether_addr dst;


	/* Forge dst mac addr */
	dst.addr_bytes[0] = 0x01;
	dst.addr_bytes[1] = 0x00;
	dst.addr_bytes[2] = 0x5e;
	dst.addr_bytes[3] = ((uint8_t *)&ip)[1] & 0x7f ; /* 16 - 24 */
	dst.addr_bytes[4] = ((uint8_t *)&ip)[2]; /* 9-16 */
	dst.addr_bytes[5] = ((uint8_t *)&ip)[3]; /* 1-8 */
	return dst;
}

/**
 * Is the given IP in the multicast range ?
 *
 * http://www.iana.org/assignments/multicast-addresses/multicast-addresses.xhtml
 *
 * @param	ip the ip to check, must be in network order (big indian)
 * @return	1 if true, 0 if false
 */
static inline bool is_multicast_ip(uint32_t ip)
{
	uint8_t byte;

	byte = ((uint8_t *) &ip)[0];

	if (byte >= 224 && byte <= 239)
		return true;

	return false;
}

/**
 * Build the VXLAN header
 *
 * @param	header pointer to the VTEP header
 * @param	vni 24 bit Virtual Network Identifier
 */
static inline void vxlan_build(struct vxlan_hdr *header, uint32_t vni)
{
	/* mark the VNI as valid */
	header->vx_flags = rte_cpu_to_be_32(VTEP_I_FLAG);

	/**
	 * We have checked the VNI validity at VNI setup so reserved byte will
	 * be zero.
	 */
	header->vx_vni = vni;
}

/**
 * Compute a hash on the ethernet header that will be used for
 * ECMP/load-balancing
 *
 * @param	eth_hdr the ethernet header that must be hashed
 * @return	the hash of 16 first bytes of the ethernet frame
 */
static inline  uint16_t ethernet_header_hash(struct ether_hdr *eth_hdr)
{
	uint64_t *data = (uint64_t *) eth_hdr;
	/* TODO: set the seed */
	uint64_t result = hash_64(data, 8, 0);

	result |= hash_64(data + 1, 8, 0);

	return (uint16_t) result;
}

/**
 * Compute the udp source port for ECMP/load-balancing
 *
 * @param	ether_hash the ethernet hash to use as a basis for the src port
 * @return	the resulting UDP source port
 */
static inline uint16_t src_port_compute(uint16_t ether_hash)
{
	return (ether_hash % UDP_PORT_RANGE) + UDP_MIN_PORT;
}

/**
 * Build the UDP header
 *
 * @param	udp_hdr pointer to the UDP header
 * @param	inner_eth_hdr pointer to the ethernet frame to encapsulate
 * @param	dst_port UDP destination port
 * @param	datagram_len length of the UDP datagram
 */
static inline void udp_build(struct udp_hdr *udp_hdr,
		      struct ether_hdr *inner_eth_hdr,
		      uint16_t dst_port,
		      uint16_t datagram_len)
{
	uint32_t ether_hash = ethernet_header_hash(inner_eth_hdr);
	uint16_t src_port = src_port_compute(ether_hash);

	udp_hdr->src_port = rte_cpu_to_be_16(src_port);
	udp_hdr->dst_port = rte_cpu_to_be_16(dst_port);
	udp_hdr->dgram_len = rte_cpu_to_be_16(datagram_len);

	/* UDP checksum SHOULD be transmited as zero */
}

/**
 * Build the IP header
 *
 * @param	ip_hdr pointer to the ip header to build
 * @param	src_ip the source IP
 * @param	dst_ip the destination IP
 * @param	datagram_len the lenght of the datagram including the header
 */
static inline void ip_build(struct vtep_state *state, struct ipv4_hdr *ip_hdr,
	      uint32_t src_ip, uint32_t dst_ip, uint16_t datagram_len)
{
	ip_hdr->version_ihl = 0x45;

	/* TOS is zero (routine) */

	ip_hdr->total_length = rte_cpu_to_be_16(datagram_len);

	/* Set the packet id and increment it */
	ip_hdr->packet_id =
		rte_cpu_to_be_16(rte_atomic16_add_return(&state->packet_id, 1));

	/* the implementation do not use neither DF nor MF */

	/* packet are not fragmented so Fragment Offset is zero */

	/* recommended TTL value */
	ip_hdr->time_to_live = 64;

	ip_hdr->hdr_checksum = 0;

	/* This IP datagram encapsulate and UDP packet */
	ip_hdr->next_proto_id = UDP_PROTOCOL_NUMBER;

	/* the header checksum computation is to be offloaded in the NIC */

	ip_hdr->src_addr = src_ip;
	ip_hdr->dst_addr = dst_ip;
	ip_hdr->hdr_checksum = rte_ipv4_cksum(ip_hdr);
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
	eth_hdr->ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
}

static inline uint16_t udp_overhead(void)
{
	return sizeof(struct vxlan_hdr) + sizeof(struct udp_hdr);
}

static inline uint16_t ip_overhead(void)
{
	return udp_overhead() + sizeof(struct ipv4_hdr);
}

static inline int vtep_header_prepend(struct vtep_state *state,
				      struct rte_mbuf *pkt,
				      struct vtep_port *port,
				      struct dest_addresses *entry,
				      int unicast,
				      struct pg_error **errp)
{
	struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
	uint16_t packet_len = rte_pktmbuf_data_len(pkt);
	/* TODO: double check this value */
	struct ether_addr *dst_mac;
	struct headers *headers;
	struct ether_addr dst;
	uint32_t dst_ip;

	headers = (struct headers *) rte_pktmbuf_prepend(pkt, HEADERS_LENGTH);

	if (unlikely(!headers)) {
		*errp = pg_error_new("No enough headroom to add VTEP headers");
		return 0;
	}

	vxlan_build(&headers->vxlan, port->vni);
	udp_build(&headers->udp, eth_hdr, state->dst_port,
		  packet_len + udp_overhead());

	/* select destination IP and MAC address */
	if (likely(unicast)) {
		dst_mac = &entry->mac;
		dst_ip = entry->ip;
	} else {
		if (!(state->flags & NO_INNERMAC_CKECK))
			do_add_mac(port, &eth_hdr->s_addr);

		dst_ip = port->multicast_ip;
		dst = multicast_get_dst_addr(dst_ip);
		dst_mac = &dst;
	}
	ip_build(state, &headers->ipv4, state->ip, dst_ip,
		 packet_len + ip_overhead());
	ethernet_build(&headers->ethernet, &state->mac, dst_mac);

	return 1;
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
		/* struct ether_hdr *eth_hdr; */
		/* uint32_t dst_ip; */
		uint64_t bit;
		struct rte_mbuf *pkt;
		int unicast;
		uint16_t i;
		struct rte_mbuf *tmp;

		pg_low_bit_iterate_full(pkts_mask, bit, i);

		pkt = pkts[i];
		/* must we encapsulate in an unicast VTEP header */
		unicast = !is_multicast_ether_addr(dst_key_ptr(pkt));

		/* pick up the right destination ip */
		if (likely(unicast)) {
			struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(
				pkts[i],
				struct ether_hdr *);

			entry = pg_mac_table_elem_get(
				&port->mac_to_dst,
				*((union pg_mac *)&eth_hdr->d_addr),
				struct dest_addresses);
			if (unlikely(!entry))
				unicast = 0;
		}

		if (unlikely(!(state->flags & NO_COPY)))
			tmp = rte_pktmbuf_clone(pkts[i], mp);
		else
			tmp = pkts[i];
		if (unlikely(!tmp))
			return 0;

		if (unlikely(!vtep_header_prepend(state, tmp, port,
						  entry, unicast, errp))) {
			rte_pktmbuf_free(tmp);
			return 0;
		}
		state->pkts[i] = tmp;

	}

	return 1;
}

static inline int to_vtep(struct pg_brick *brick, enum pg_side from,
		    uint16_t edge_index, struct rte_mbuf **pkts,
		    uint16_t nb, uint64_t pkts_mask,
		    struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct vtep_port *port = &state->ports[edge_index];
	int ret;

	/* if the port VNI is not set up ignore the packets */
	if (!unlikely(port->multicast_ip))
		return 1;


	ret = vtep_encapsulate(state, port, pkts, pkts_mask, errp);

	if (unlikely(!ret))
		return 0;

	ret =  pg_brick_side_forward(s, from, state->pkts, nb, pkts_mask, errp);
	if (!(state->flags & NO_COPY))
		pg_packets_free(state->pkts, pkts_mask);
	return ret;
}

static inline void add_dst_iner_macs(struct vtep_state *state,
				    struct vtep_port *port,
				    struct rte_mbuf **pkts,
				    struct headers **hdrs,
				    uint64_t pkts_mask,
				    uint64_t multicast_mask)
{
	uint64_t mask;
	uint64_t bit;
	union pg_mac tmp;

	tmp.mac = 0;

	for (mask = multicast_mask; mask;) {
		int i;
		struct dest_addresses dst;
		struct ether_hdr *pkt_addr;

		pg_low_bit_iterate_full(mask, bit, i);

		pkt_addr = rte_pktmbuf_mtod(pkts[i], struct ether_hdr *);
		ether_addr_copy(&hdrs[i]->ethernet.s_addr, &dst.mac);
		dst.ip = hdrs[i]->ipv4.src_addr;
		memcpy(tmp.bytes, &pkt_addr->s_addr.addr_bytes, 6);
		pg_mac_table_elem_set(&port->mac_to_dst, tmp, &dst,
				      sizeof(struct dest_addresses));
	}
}

static inline int from_vtep_failure(struct rte_mbuf **pkts,
					     uint64_t pkts_mask,
					     int no_copy)
{
	if (!no_copy)
		pg_packets_free(pkts, pkts_mask);
	return 0;
}

static inline void check_multicasts_pkts(struct rte_mbuf **pkts, uint64_t mask,
					 struct headers **hdrs,
					 uint64_t *multicast_mask,
					 uint64_t *computed_mask)
{
	for (*multicast_mask = 0, *computed_mask = 0; mask;) {
		int i;

		pg_low_bit_iterate(mask, i);
		hdrs[i] = rte_pktmbuf_mtod(pkts[i], struct headers *);
		/* This should not hapen */
		if (unlikely(hdrs[i]->ethernet.ether_type !=
			     rte_cpu_to_be_16(ETHER_TYPE_IPv4) ||
			     hdrs[i]->ipv4.next_proto_id != 17 ||
			     hdrs[i]->vxlan.vx_flags !=
			     rte_cpu_to_be_32(VTEP_I_FLAG)))
			continue;
		if (is_multicast_ip(hdrs[i]->ipv4.dst_addr))
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

			if (unlikely(!(state->flags & NO_COPY))) {
				struct rte_mempool *mp = pg_get_mempool();

				tmp = rte_pktmbuf_clone(pkts[j], mp);
			} else {
				tmp = pkts[j];
			}
			if (unlikely(!tmp))
				return 0;
			out_pkts[j] = tmp;
			if (!rte_pktmbuf_adj(out_pkts[j], HEADERS_LENGTH))
				return 0;
			vni_mask |= (1LLU << j);
		}
	}
	return vni_mask;
}

static inline int from_vtep(struct pg_brick *brick, enum pg_side from,
		      uint16_t edge_index, struct rte_mbuf **pkts,
		      uint16_t nb, uint64_t pkts_mask,
		      struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	int i;
	struct headers *hdrs[64];
	struct rte_mbuf **out_pkts = state->pkts;
	uint64_t multicast_mask;
	uint64_t computed_pkts;

	check_multicasts_pkts(pkts, pkts_mask, hdrs,
			      &multicast_mask, &computed_pkts);

	pkts_mask &= computed_pkts;
	for (i = 0; i < s->nb; ++i) {
		struct vtep_port *port = &state->ports[i];
		uint64_t hitted_mask = 0;
		uint64_t vni_mask;

		if (!pkts_mask)
			break;
		/* Decaspulate and check the vni*/
		vni_mask = check_and_clone_vni_pkts(state, pkts, pkts_mask,
						    hdrs, port, out_pkts);
		if (!vni_mask)
			continue;

		pkts_mask ^= vni_mask;
		if (state->flags & NO_INNERMAC_CKECK) {
			hitted_mask = vni_mask;
		} else {
			PG_FOREACH_BIT(vni_mask, it) {
				void *entry;
				struct ether_hdr *eth_hdr = rte_pktmbuf_mtod(
					out_pkts[i],
					struct ether_hdr *);

				entry = pg_mac_table_ptr_get(
					&port->known_mac,
					*((union pg_mac *)&eth_hdr->d_addr));

				if (entry)
					hitted_mask |= ONE64 << it;
			}
		}

		if (hitted_mask) {
			add_dst_iner_macs(state, port, out_pkts, hdrs,
					  hitted_mask, multicast_mask);

			if (unlikely(!pg_brick_burst(s->edges[i].link,
						  from,
						  i, out_pkts, nb,
						  hitted_mask,
						  errp)))
				return from_vtep_failure(out_pkts,
							 vni_mask,
							 state->flags &
							 NO_COPY);
		}

		if (unlikely(!(state->flags & NO_COPY)))
			pg_packets_free(out_pkts, vni_mask);
	}
	return 1;
}

static int vtep_burst(struct pg_brick *brick, enum pg_side from,
			uint16_t edge_index, struct rte_mbuf **pkts,
			uint16_t nb, uint64_t pkts_mask,
			struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);

	/* if pkts come from the outside,
	 * so the pkts are entering in the vtep */
	if (from == state->output)
		return from_vtep(brick, from, edge_index,
				  pkts, nb, pkts_mask, errp);
	else
		return to_vtep(brick, from, edge_index,
				pkts, nb, pkts_mask, errp);
}

static int vtep_init(struct pg_brick *brick,
		      struct pg_brick_config *config, struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	struct vtep_config *vtep_config;
	uint16_t max;

	if (!config) {
		*errp = pg_error_new("config is NULL");
		return 0;
	}

	if (!config->brick_config) {
		*errp = pg_error_new("config->brick_config is NULL");
		return 0;
	}

	vtep_config = config->brick_config;

	state->output = vtep_config->output;
	if (pg_side_get_max(brick, state->output) != 1) {
		*errp = pg_error_new("brick %s number of output port is not 1",
				  brick->name);
		return 0;
	}
	state->ip = vtep_config->ip;
	ether_addr_copy(&vtep_config->mac, &state->mac);
	state->flags = vtep_config->flags;

	rte_atomic16_set(&state->packet_id, 0);

	if (pg_error_is_set(errp))
		return 0;

	if (pg_error_is_set(errp))
		return 0;

	/* do a lazy allocation of the VTEP ports: the code will init them
	 * at VNI port add
	 */
	max = pg_side_get_max(brick, pg_flip_side(state->output));
	state->ports = g_new0(struct vtep_port, max);
	state->masks = g_new0(uint64_t, max);

	brick->burst = vtep_burst;

	return 1;
}

static struct pg_brick_config *vtep_config_new(const char *name,
					       uint32_t west_max,
					       uint32_t east_max,
					       enum pg_side output,
					       uint32_t ip,
					       struct ether_addr mac,
					       int flags)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct vtep_config *vtep_config = g_new0(struct vtep_config, 1);

	vtep_config->output = output;
	vtep_config->ip = ip;
	vtep_config->flags = flags;
	ether_addr_copy(&mac, &vtep_config->mac);
	config->brick_config = vtep_config;
	return pg_brick_config_init(config, name, west_max,
				    east_max, PG_MULTIPOLE);
}

struct pg_brick *pg_vtep_new(const char *name, uint32_t west_max,
		       uint32_t east_max, enum pg_side output,
		       uint32_t ip, struct ether_addr mac,
		       int flags, struct pg_error **errp)
{
	struct pg_brick_config *config = vtep_config_new(name, west_max,
							 east_max, output,
							 ip, mac, flags);
	struct pg_brick *ret = pg_brick_new("vtep", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static void vtep_destroy(struct pg_brick *brick, struct pg_error **errp)
{
}

/**
 * Is the given VNI valid
 *
 * @param	vni the VNI to check
 * @return	1 if true, 0 if false
 */
static bool is_vni_valid(uint32_t vni)
{
	/* VNI is coded on 24 bits */
	return vni <= (UINT32_MAX >> 8);
}

static inline uint16_t igmp_checksum(struct igmp_hdr *msg, size_t size)
{
	uint16_t sum = 0;

	sum = rte_raw_cksum(msg, sizeof(struct igmp_hdr));
	return ~sum;
}

static void multicast_subscribe(struct vtep_state *state,
				struct vtep_port *port,
				uint32_t multicast_ip,
				struct pg_error **errp)
{
	multicast_internal(state, port, multicast_ip, errp, IGMP_SUBSCRIBE);
}

static void multicast_unsubscribe(struct vtep_state *state,
				  struct vtep_port *port,
				  uint32_t multicast_ip,
				  struct pg_error **errp)
{
	multicast_internal(state, port, multicast_ip, errp, IGMP_UNSUBSCRIBE);
}

#undef UINT64_TO_MAC

static void do_add_vni(struct vtep_state *state, uint16_t edge_index,
		       int32_t vni, uint32_t multicast_ip,
		       struct pg_error **errp)
{
	struct vtep_port *port = &state->ports[edge_index];

	/* TODO: return 1 ? */
	g_assert(!port->vni);
	g_assert(!port->multicast_ip);
	vni = rte_cpu_to_be_32(vni);

	port->vni = vni;
	port->multicast_ip = multicast_ip;

	pg_mac_table_init(&port->mac_to_dst);
	pg_mac_table_init(&port->known_mac);

	multicast_subscribe(state, port, multicast_ip, errp);
}

void pg_vtep_add_vni(struct pg_brick *brick,
		   struct pg_brick *neighbor,
		   uint32_t vni, uint32_t multicast_ip,
		   struct pg_error **errp)
{
	struct vtep_state *state = pg_brick_get_state(brick, struct vtep_state);
	enum pg_side side = pg_flip_side(state->output);
	uint16_t i;
	int found;
	struct ether_addr mac = {{0xff, 0xff, 0xff,
				  0xff, 0xff, 0xff} };

	if (!brick) {
		*errp = pg_error_new("brick is NULL");
		return;
	}

	if (!neighbor) {
		*errp = pg_error_new("VTEP brick is NULL");
		return;
	}

	if (!is_vni_valid(vni)) {
		*errp = pg_error_new("Invalid VNI");
		return;
	}

	if (!is_multicast_ip(multicast_ip)) {
		*errp = pg_error_new(
			"Provided IP is not in the multicast range");
		return;
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
		return;
	}

	do_add_vni(state, i, vni, multicast_ip, errp);
	do_add_mac(&state->ports[i], &mac);
}

static void do_remove_vni(struct vtep_state *state,
		   uint16_t edge_index, struct pg_error **errp)
{
	struct vtep_port *port = &state->ports[edge_index];

	multicast_unsubscribe(state, port, port->multicast_ip, errp);

	if (pg_error_is_set(errp))
		return;

	/* Do the hash destroy at the end since it's the less idempotent */
	pg_mac_table_free(&port->known_mac);
	pg_mac_table_free(&port->mac_to_dst);

	/* clear for next user */
	memset(port, 0, sizeof(struct vtep_port));
}

static inline void do_add_mac(struct vtep_port *port, struct ether_addr *mac)
{
	union pg_mac tmp;

	tmp.mac = 0;
	rte_memcpy(tmp.bytes, mac->addr_bytes, 6);
	pg_mac_table_ptr_set(&port->known_mac, tmp, (void *)1);
}

static void multicast_internal(struct vtep_state *state,
			       struct vtep_port *port,
			       uint32_t multicast_ip,
			       struct pg_error **errp,
			       enum operation action)
{
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf *pkt[1];
	struct multicast_pkt *hdr;
	struct ether_addr dst = multicast_get_dst_addr(multicast_ip);

	if (!is_multicast_ip(multicast_ip))
		goto error_invalid_address;

	/* The all-systems group (224.0.0.1) is handled as a special case. */
  /* The host never sends a report for that group */
	if (multicast_ip == IPv4(224, 0, 0, 1))
		goto error_invalid_address;

	/* Allocate a memory buffer to hold an IGMP message */
	pkt[0] = rte_pktmbuf_alloc(mp);
	if (!pkt[0]) {
		pg_error_new("Packet allocation faild");
		return;
	}

	/* Point to the beginning of the IGMP message */
	hdr = (struct multicast_pkt *) rte_pktmbuf_append(pkt[0],
							  IGMP_PKT_LEN);

	ether_addr_copy(&state->mac, &hdr->ethernet.s_addr);
	/* Because of the conversion from le to be, we need to
	 * skip the first
	 * byte of dst when making the copy*/
	ether_addr_copy(&dst,
			&hdr->ethernet.d_addr);
	hdr->ethernet.ether_type = rte_cpu_to_be_16(ETHER_TYPE_IPv4);

	/* 4-5 = 0x45 */
	hdr->ipv4.version_ihl = 0x45;
	hdr->ipv4.type_of_service = 0;
	hdr->ipv4.total_length = rte_cpu_to_be_16(sizeof(struct ipv4_hdr) +
						  sizeof(struct igmp_hdr));
	hdr->ipv4.packet_id = 0;
	hdr->ipv4.fragment_offset = 0;
	hdr->ipv4.time_to_live = 1;
	hdr->ipv4.next_proto_id = IGMP_PROTOCOL_NUMBER;
	hdr->ipv4.hdr_checksum = 0;
	hdr->ipv4.src_addr = state->ip;
	hdr->ipv4.hdr_checksum = rte_ipv4_cksum(&hdr->ipv4);

	/* Version 2 Membership Report = 0x16 */
	hdr->igmp.type = action;
	hdr->igmp.max_resp_time = 0;
	hdr->igmp.checksum = 0;
	hdr->igmp.group_addr = multicast_ip;
	hdr->igmp.checksum = igmp_checksum(&hdr->igmp, sizeof(struct igmp_hdr));
	switch (action) {
	case IGMP_SUBSCRIBE:
		hdr->ipv4.dst_addr = multicast_ip;
		break;
	case IGMP_UNSUBSCRIBE:
		hdr->ipv4.dst_addr = IPv4(224, 0, 0, 2);
		break;
	default:
		pg_error_new("action not handle");
		goto error;
	}

	pg_brick_side_forward(&state->brick.sides[state->output],
			      pg_flip_side(state->output),
			      pkt, 1, pg_mask_firsts(1), errp);

error:
	rte_pktmbuf_free(pkt[0]);
	return;
error_invalid_address:
	pg_error_new("invalide multicast adress");
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

static struct pg_brick_ops vtep_ops = {
	.name		= "vtep",
	.state_size	= sizeof(struct vtep_state),

	.init		= vtep_init,
	.destroy	= vtep_destroy,

	.unlink		= pg_brick_generic_unlink,

	.unlink_notify  = vtep_unlink_notify,
};

#undef HEADERS_LENGTH

pg_brick_register(vtep, &vtep_ops);
