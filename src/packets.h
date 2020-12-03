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
 */

#ifndef _PG_PACKETS_H
#define _PG_PACKETS_H

#include <rte_config.h>
#include <rte_mbuf.h>
#include <rte_port.h>

#include <packetgraph/errors.h>

#define HASH_ENTRIES		(1024 * 32)
#define HASH_KEY_SIZE		8

#define APP_METADATA_OFFSET(offset) (sizeof(struct rte_mbuf) + (offset))

#define PKT_GET_METADATA(pkt, offset)					\
	(RTE_MBUF_METADATA_UINT8_PTR(pkt,				\
				     APP_METADATA_OFFSET(offset)))

/**
 * Made as a macro for performance reason since a function would imply a 10%
 * performance hit and a function in a separate module would not be inlined.
 */
#define dst_key_ptr(pkt)					\
	((struct ether_addr *) PKT_GET_METADATA(pkt, 0))

/**
 * Made as a macro for performance reason since a function would imply a 10%
 * performance hit and a function in a separate module would not be inlined.
 */
#define src_key_ptr(pkt)						\
	((struct ether_addr *) PKT_GET_METADATA(pkt, HASH_KEY_SIZE))


struct ether_addr;

/**
 * @return: a newly allocated array of packets,
 *	    caller must free each packets, and the array
 */
struct rte_mbuf **pg_packets_create(uint64_t pkts_mask);

struct rte_mbuf **pg_packets_append_blank(struct rte_mbuf **pkts,
					  uint64_t pkts_mask,
					  uint16_t len);

struct rte_mbuf **pg_packets_prepend_blank(struct rte_mbuf **pkts,
					  uint64_t pkts_mask,
					  uint16_t len);

struct rte_mbuf **pg_packets_append_buf(struct rte_mbuf **pkts,
					uint64_t pkts_mask,
					const void *buf,
					size_t len);

struct rte_mbuf **pg_packets_prepend_buf(struct rte_mbuf **pkts,
					 uint64_t pkts_mask,
					 const void *buf,
					 size_t len);

struct rte_mbuf **pg_packets_append_str(struct rte_mbuf **pkts,
					uint64_t pkts_mask,
					const char *str);

struct rte_mbuf **pg_packets_prepend_str(struct rte_mbuf **pkts,
					 uint64_t pkts_mask,
					 const char *str);

struct rte_mbuf **pg_packets_append_ether(struct rte_mbuf **pkts,
					  uint64_t pkts_mask,
					  struct ether_addr *src_mac,
					  struct ether_addr *dst_mac,
					  uint16_t ether_type);

struct rte_mbuf **pg_packets_prepend_ether(struct rte_mbuf **pkts,
					   uint64_t pkts_mask,
					   struct ether_addr *src_mac,
					   struct ether_addr *dst_mac,
					   uint16_t ether_type);

struct rte_mbuf **pg_packets_append_ipv4(struct rte_mbuf **pkts,
					 uint64_t pkts_mask,
					 uint32_t src_ip, uint32_t dst_ip,
					 uint16_t datagram_len,
					 uint8_t proto);

struct rte_mbuf **pg_packets_append_ipv6(struct rte_mbuf **pkts,
					 uint64_t pkts_mask,
					 uint8_t *src_ip, uint8_t *dst_ip,
					 uint16_t datagram_len,
					 uint8_t proto);

struct rte_mbuf **pg_packets_prepend_ipv4(struct rte_mbuf **pkts,
					 uint64_t pkts_mask,
					 uint32_t src_ip, uint32_t dst_ip,
					 uint16_t datagram_len,
					 uint8_t proto);

struct rte_mbuf **pg_packets_append_udp(struct rte_mbuf **pkts,
					uint64_t pkts_mask,
					uint16_t src_port, uint16_t dst_port,
					uint16_t datagram_len);

struct rte_mbuf **pg_packets_prepend_udp(struct rte_mbuf **pkts,
					 uint64_t pkts_mask,
					 uint16_t src_port, uint16_t dst_port,
					 uint16_t datagram_len);

struct rte_mbuf **pg_packets_append_vxlan(struct rte_mbuf **pkts,
					  uint64_t pkts_mask,
					  uint32_t vni);

struct rte_mbuf **pg_packets_prepend_vxlan(struct rte_mbuf **pkts,
					   uint64_t pkts_mask,
					   uint32_t vni);



int pg_packets_pack(struct rte_mbuf **dst,
		    struct rte_mbuf **src,
		    uint64_t pkts_mask);

void pg_packets_incref(struct rte_mbuf **pkts, uint64_t pkts_mask);

void pg_packets_free(struct rte_mbuf **pkts, uint64_t pkts_mask);

void pg_packets_forget(struct rte_mbuf **pkts, uint64_t pkts_mask);

void pg_packets_prefetch(struct rte_mbuf **pkts, uint64_t pkts_mask);

#endif /* _PG_PACKETS_H */
