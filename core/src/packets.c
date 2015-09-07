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
 */

#include <rte_config.h>
#include <rte_table_hash.h>
#include <rte_table.h>
#include <rte_table_hash.h>
#include <rte_udp.h>
#include <rte_ip.h>
#include <rte_ether.h>

#include <glib.h>
#include <string.h>

#include <packetgraph/packets.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/utils/mempool.h>

struct rte_mbuf **pg_packets_create(uint64_t pkts_mask)
{
	/*For now we allocate the maximum number of pkts*/
	struct rte_mbuf **ret = g_new0(struct rte_mbuf *, 64);

	if (!ret)
		return NULL;
	pg_foreach_bit(pkts_mask, i) {
		ret[i] = rte_pktmbuf_alloc(pg_get_mempool());
	}
	return ret;
}

struct rte_mbuf **pg_packets_append_str(struct rte_mbuf **pkts,
					uint64_t pkts_mask,
					const char *str)
{
	char *tmp;
	
	pg_foreach_bit(pkts_mask, j) {
		if (!pkts[j])
			continue;
		tmp = rte_pktmbuf_append(pkts[j], strlen(str));
		strcpy(tmp, str);
	}
	return pkts;
}


struct rte_mbuf **pg_packets_append_ipv4(struct rte_mbuf **pkts,
					 uint64_t pkts_mask,
					 uint32_t src_ip, uint32_t dst_ip,
					 uint16_t datagram_len, uint8_t proto)
{
	struct ipv4_hdr	ip_hdr;
	char *tmp;
	
	pg_foreach_bit(pkts_mask, j) {
		if (!pkts[j])
			continue;
		ip_hdr.version_ihl = 0x45;
		ip_hdr.total_length = rte_cpu_to_be_16(datagram_len);
		ip_hdr.packet_id = 0;
		ip_hdr.time_to_live = 64;
		ip_hdr.hdr_checksum = 0;
		ip_hdr.next_proto_id = proto;
		ip_hdr.src_addr = src_ip;
		ip_hdr.dst_addr = dst_ip;
		ip_hdr.hdr_checksum = rte_ipv4_cksum(&ip_hdr);

		tmp = rte_pktmbuf_append(pkts[j], sizeof(ip_hdr));
		memcpy(tmp, &ip_hdr, sizeof(ip_hdr));
	}
	return pkts;
}

struct rte_mbuf **pg_packets_append_udp(struct rte_mbuf **pkts,
					uint64_t pkts_mask,
					uint16_t src_port, uint16_t dst_port,
					uint16_t datagram_len)
{
	struct udp_hdr udp_hdr;
	char *tmp;
	
	pg_foreach_bit(pkts_mask, j) {
		if (!pkts[j])
			continue;	
		udp_hdr.src_port = rte_cpu_to_be_16(src_port);
		udp_hdr.dst_port = rte_cpu_to_be_16(dst_port);
		udp_hdr.dgram_len = rte_cpu_to_be_16(datagram_len);
		udp_hdr.dgram_cksum = 0;
		tmp = rte_pktmbuf_append(pkts[j], sizeof(udp_hdr));
		memcpy(tmp, &udp_hdr, sizeof(udp_hdr));
	}
	return pkts;
}

#define VTEP_I_FLAG		0x08000000

struct rte_mbuf **pg_packets_append_vxlan(struct rte_mbuf **pkts,
					  uint64_t pkts_mask,
					  uint32_t vni)
{
	struct vxlan_hdr vx_hdr;
	char *tmp;
	
	pg_foreach_bit(pkts_mask, j) {
		if (!pkts[j])
			continue;

		/* mark the VNI as valid */
		vx_hdr.vx_flags = rte_cpu_to_be_32(VTEP_I_FLAG);
		vx_hdr.vx_vni = rte_cpu_to_be_32(vni);
		tmp = rte_pktmbuf_append(pkts[j], sizeof(vx_hdr));
		memcpy(tmp, &vx_hdr, sizeof(vx_hdr));
	}
	return pkts;
}

#undef VTEP_I_FLAG

struct rte_mbuf **pg_packets_append_ether(struct rte_mbuf **pkts,
					 uint64_t pkts_mask,
					 struct ether_addr *src_mac,
					 struct ether_addr *dst_mac,
					 uint16_t ether_type)
{
	struct ether_hdr eth_hdr;
	char *tmp;
	
	pg_foreach_bit(pkts_mask, j) {
		if (!pkts[j])
			continue;
		ether_addr_copy(src_mac, &eth_hdr.s_addr);
		ether_addr_copy(dst_mac, &eth_hdr.d_addr);

		eth_hdr.ether_type = rte_cpu_to_be_16(ether_type);
		tmp = rte_pktmbuf_append(pkts[j], sizeof(eth_hdr));
		memcpy(tmp, &eth_hdr, sizeof(eth_hdr));
	}
	return pkts;
}

/**
 * This function copy and pack a source packet array into a destination array
 * It uses the packet mask to know which packet are to copy.
 *
 * @param	dst the destination packet array
 * @param	src the source packet array
 * @param	pkts_mask the packing mask
 * @return	the number of packed packets
 */
int pg_packets_pack(struct rte_mbuf **dst,
		    struct rte_mbuf **src,
		    uint64_t pkts_mask)
{
	int i;

	for (i = 0; pkts_mask; i++) {
		uint16_t j;

		pg_low_bit_iterate(pkts_mask, j);
		dst[i] = src[j];
	}
	return i;
}

/**
 * This function selectively increment the refcount of member of a packet array
 *
 * @param	pkts the packet array
 * @param	pkts_mask mask of packet to increment refcount of
 */
void pg_packets_incref(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for (; pkts_mask;) {
		uint16_t i;

		pg_low_bit_iterate(pkts_mask, i);
		rte_mbuf_refcnt_update(pkts[i], 1);
	}
}

/**
 * This function selectively free members of a packet array
 *
 * @param	pkts the packet array
 * @param	pkts_mask mask of packet to free
 */
void pg_packets_free(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for (; pkts_mask;) {
		uint16_t i;

		pg_low_bit_iterate(pkts_mask, i);
		rte_pktmbuf_free(pkts[i]);
	}
}

/**
 * This function selectively set to NULL references of a packet array
 *
 * @param	pkts the packet array
 * @param	pkts_mask mask of packet to forget
 */
void pg_packets_forget(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for (; pkts_mask;) {
		uint16_t i;

		pg_low_bit_iterate(pkts_mask, i);
		pkts[i] = NULL;
	}
}


void pg_packets_prefetch(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	uint64_t mask;

	for (mask = pkts_mask; mask; ) {
		struct rte_mbuf *pkt;
		uint16_t i;

		pg_low_bit_iterate(mask, i);
		pkt = pkts[i];
		rte_prefetch0(pkt);
	}
	for (mask = pkts_mask; mask; ) {
		struct ether_hdr *eth_hdr;
		struct rte_mbuf *pkt;
		uint16_t i;

		pg_low_bit_iterate(mask, i);

		pkt = pkts[i];
		eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
		rte_prefetch0(eth_hdr);
	}
}

int pg_packets_prepare_hash_keys(struct rte_mbuf **pkts,
				 uint64_t pkts_mask,
				 struct pg_error **errp)
{
	for ( ; pkts_mask; ) {
		struct ether_hdr *eth_hdr;
		struct rte_mbuf *pkt;
		uint16_t i;

		pg_low_bit_iterate(pkts_mask, i);

		pkt = pkts[i];

		eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);

		if (unlikely(pkt->data_off < HASH_KEY_SIZE * 2)) {
			*errp = pg_error_new("Not enough headroom space");
			return 0;
		}

		memset(dst_key_ptr(pkt), 0, HASH_KEY_SIZE * 2);
		ether_addr_copy(&eth_hdr->d_addr, dst_key_ptr(pkt));
		ether_addr_copy(&eth_hdr->s_addr, src_key_ptr(pkt));
		rte_prefetch0(dst_key_ptr(pkt));
	}

	return 1;
}

void pg_packets_clear_hash_keys(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for ( ; pkts_mask; ) {
		struct rte_mbuf *pkt;
		uint16_t i;

		pg_low_bit_iterate(pkts_mask, i);

		pkt = pkts[i];

		g_assert(pkt->data_off >= HASH_KEY_SIZE * 2);

		memset(dst_key_ptr(pkt), 0, HASH_KEY_SIZE * 2);
	}
}
