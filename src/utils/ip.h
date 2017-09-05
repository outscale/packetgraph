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

#ifndef _PG_UTILS_IP_H
#define _PG_UTILS_IP_H

#include <arpa/inet.h>
#include <rte_ether.h>
#include <rte_ip.h>

union pg_ipv6_addr {
	uint8_t word8[16];
	uint16_t word16[8];
	uint32_t word32[4];
	uint64_t word64[2];
} __attribute__((__packed__));

#define PG_IP_GENERIC_IPV6(callback)		\
	uint8_t[16] : callback,			\
		uint8_t * : callback

#define pg_ip_from_str(ip, src)						\
	(inet_pton(							\
		_Generic((ip), PG_IP_GENERIC_IPV6(AF_INET6),		\
			 uint32_t : AF_INET),				\
		src,							\
		_Generic((ip), PG_IP_GENERIC_IPV6(ip), uint32_t : &(ip)) \
		))

#define PG_UDP_PROTOCOL_NUMBER 17
#define PG_TCP_PROTOCOL_NUMBER 6

#define PG_IPV6_HALF_FORMAT "%02x%02x:%02x%02x:%02x%02x:%02x%02x"
#define PG_IPV6_FORMAT PG_IPV6_HALF_FORMAT":"PG_IPV6_HALF_FORMAT

/**
 * Compute a precalculate semi ipv4 checksum, that need to be use with
 * pg_ipv4_udp_cksum
 * @return the semi checksum
 */
static inline uint32_t pg_ipv4_init_cksum(uint32_t src_ip)
{
	struct ipv4_hdr ip_hdr;

	ip_hdr.version_ihl = 0x45;
	ip_hdr.type_of_service = 0;
	ip_hdr.total_length = 0;
	ip_hdr.packet_id = 0;
	ip_hdr.time_to_live = 64;
	ip_hdr.fragment_offset = 0;
	ip_hdr.hdr_checksum = 0;
	ip_hdr.next_proto_id = PG_UDP_PROTOCOL_NUMBER;
	ip_hdr.src_addr = src_ip;
	ip_hdr.dst_addr = 0;
	return __rte_raw_cksum(&ip_hdr, sizeof(ip_hdr), 0);
}

/**
 * Assume that all udp ip packets are similar with only
 * packet_len, packet_id and dst_addr different from other packets
 * from this assumption, do a fast checksum
 *
 * @param base_sum_ipv4 semi checksum precalculate by pg_ipv4_init_cksum
 * @param pkt_len packet len
 * @param id packet_id
 * @param dts_ip destination ip
 * @return the checksum
 */
static inline uint16_t pg_ipv4_udp_cksum(uint32_t base_sum_ipv4,
					 uint16_t pkt_len, uint16_t id,
					 uint32_t dts_ip)
{
	union {
		uint32_t ip;
		uint16_t ipp[2];
	} __attribute__((__packed__)) ip = { .ip = dts_ip };
	uint16_t sum = __rte_raw_cksum_reduce(base_sum_ipv4 + pkt_len + id +
					      ip.ipp[0] + ip.ipp[1]);

	if (unlikely(sum == 0xffff))
		return 0xffff;
	return ~sum;
}

static inline const char *pg_ipv6_to_str(const uint8_t *ip)
{
	static char static_bufs[16][64];
	static int cur;
	const char *ret = static_bufs[cur];

	sprintf(static_bufs[cur],
		PG_IPV6_FORMAT,
		ip[0], ip[1], ip[2], ip[3], ip[4], ip[5], ip[6], ip[7],
		ip[8], ip[9], ip[10], ip[11], ip[12], ip[13], ip[14], ip[15]);
	++cur;
	cur = cur % 16;
	return ret;
}

static inline int pg_ipv6_proto(struct ipv6_hdr hdr)
{
	return hdr.proto;
}

static inline int pg_ipv4_proto(struct ipv4_hdr hdr)
{
	return hdr.next_proto_id;
}

#define pg_ip_proto(ip)						\
	(_Generic((ip), struct ipv4_hdr : pg_ipv4_proto,	\
		  struct ipv6_hdr : pg_ipv6_proto) (ip))


#define pg_ip_is_same(dest, src)					\
	(_Generic((dest),						\
		uint32_t : pg_is_same_ipv4,				\
		  uint8_t * :						\
		  _Generic((src),					\
			   uint32_t : pg_ip_bug,			\
			   PG_IP_GENERIC_IPV6(pg_is_same_ipv6_tab_tab),	\
			   union pg_ipv6_addr * : pg_is_same_ipv6_tab_puni), \
		  union pg_ipv6_addr * :				\
		  _Generic((src),					\
			   uint32_t : pg_ip_bug,			\
			   PG_IP_GENERIC_IPV6(pg_is_same_ipv6_puni_tab), \
			   union pg_ipv6_addr * : pg_is_same_ipv6_puni_puni), \
		  uint8_t [16] :					\
		  _Generic((src),					\
			   uint32_t : pg_ip_bug,			\
			   PG_IP_GENERIC_IPV6(pg_is_same_ipv6_tab_tab),	\
			   union pg_ipv6_addr * : pg_is_same_ipv6_tab_puni) \
		)(dest, src))

static inline bool pg_is_same_ipv4(uint32_t a, uint32_t b)
{
	return a == b;
}

static inline bool pg_is_same_ipv6_tab_tab(uint8_t *dest,
					   uint8_t *src)
{
	return !memcmp(dest, src, sizeof(union pg_ipv6_addr));
}

static inline bool pg_is_same_ipv6_puni_tab(union pg_ipv6_addr *dest,
					    uint8_t *src)
{
	return pg_is_same_ipv6_tab_tab(dest->word8, src);
}

static inline bool pg_is_same_ipv6_tab_puni(uint8_t *dest,
					    union pg_ipv6_addr *src)
{
	return pg_is_same_ipv6_tab_tab(dest, src->word8);
}

static inline bool pg_is_same_ipv6_puni_puni(union pg_ipv6_addr *dest,
					     union pg_ipv6_addr *src)
{
	return pg_is_same_ipv6_tab_tab(dest->word8, src->word8);
}

#define pg_ip_copy(src, dest)						\
	_Generic((dest),						\
		 uint32_t * : pg_ipv4_copy,				\
		 uint8_t * :						\
		 _Generic((src),					\
			  uint32_t : pg_ip_bug,				\
			  PG_IP_GENERIC_IPV6(pg_copy_ipv6_tab_tab),	\
			  union pg_ipv6_addr * : pg_copy_ipv6_puni_tab,	\
			  union pg_ipv6_addr : pg_copy_ipv6_uni_tab),	\
		 union pg_ipv6_addr * :					\
		 _Generic((src),					\
			  uint32_t : pg_ip_bug,				\
			  PG_IP_GENERIC_IPV6(pg_copy_ipv6_tab_puni),	\
			  union pg_ipv6_addr * : pg_copy_ipv6_puni_puni, \
			  union pg_ipv6_addr : pg_copy_ipv6_uni_puni),	\
		 uint8_t [16] :						\
		 _Generic((src),					\
			  uint32_t : pg_ip_bug,				\
			  PG_IP_GENERIC_IPV6(pg_copy_ipv6_tab_tab),	\
			  union pg_ipv6_addr * : pg_copy_ipv6_puni_tab,	\
			  union pg_ipv6_addr : pg_copy_ipv6_uni_tab)	\
		)(src, dest)

static inline void pg_ip_bug(uint32_t a, ...)
{
	abort();
}

static inline void pg_ipv4_copy(uint32_t src, uint32_t *dest)
{
	*dest = src;
}

static inline void pg_copy_ipv6_tab_tab(const uint8_t *src, uint8_t *dest)
{
	rte_memcpy(dest, src, sizeof(union pg_ipv6_addr));
}

static inline void pg_copy_ipv6_puni_tab(const union pg_ipv6_addr *src,
					 uint8_t *dest)
{
	pg_copy_ipv6_tab_tab(src->word8, dest);
}

static inline void pg_copy_ipv6_tab_puni(const uint8_t *src,
					 union pg_ipv6_addr *dest)
{
	pg_copy_ipv6_tab_tab(src, dest->word8);
}

static inline void pg_copy_ipv6_puni_puni(const union pg_ipv6_addr *src,
					  union pg_ipv6_addr *dest)
{
	pg_copy_ipv6_tab_tab(src->word8, dest->word8);
}

static inline void pg_copy_ipv6_uni_tab(const union pg_ipv6_addr src,
					uint8_t *dest)
{
	pg_copy_ipv6_tab_tab(src.word8, dest);
}

static inline void pg_copy_ipv6_uni_puni(const union pg_ipv6_addr src,
					 union pg_ipv6_addr *dest)
{
	pg_copy_ipv6_tab_tab(src.word8, dest->word8);
}

#define pg_multicast_get_dst_addr(ip)					\
	_Generic((ip),							\
		 uint32_t : pg_multicast_get_dst_addr4,			\
		 union pg_ipv6_addr : pg_multicast_get_dst_addru6,	\
		 PG_IP_GENERIC_IPV6(pg_multicast_get_dst_addr6))(ip)

static inline struct ether_addr pg_multicast_get_dst_addr4(uint32_t ip)
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

static inline struct ether_addr
pg_multicast_get_dst_addr6(const uint8_t *ip)
{
	struct ether_addr dst;

	/* Forge dst mac addr */
	dst.addr_bytes[0] = 0x33;
	dst.addr_bytes[1] = 0x33;
	dst.addr_bytes[2] = ip[12];
	dst.addr_bytes[3] = ip[13];
	dst.addr_bytes[4] = ip[14];
	dst.addr_bytes[5] = ip[15];
	return dst;
}
static inline struct ether_addr
pg_multicast_get_dst_addru6(const union pg_ipv6_addr ip)
{
	return pg_multicast_get_dst_addr6(ip.word8);
}

/**
 * Is the given IP in the multicast range ?
 *
 * http://www.iana.org/assignments/multicast-addresses/multicast-addresses.xhtml
 *
 * @param	ip the ip to check, must be in network order (big indian)
 * @return	1 if true, 0 if false
 */
#define pg_is_multicast_ip(ip)					\
	(_Generic((ip),						\
		  union pg_ipv6_addr  : is_multicast_ip_uni,	\
		  union pg_ipv6_addr * : is_multicast_ip_puni,	\
		  PG_IP_GENERIC_IPV6(is_multicast_ip_tab),	\
		  uint32_t : is_multicast_ipv4)(ip))

static inline bool is_multicast_ip_uni(union pg_ipv6_addr ip)
{
	return ip.word8[0] == 0xff;
}

static inline bool is_multicast_ip_puni(union pg_ipv6_addr *ip)
{
	return ip->word8[0] == 0xff;
}

static inline bool is_multicast_ip_tab(uint8_t *ip)
{
	return ip[0] == 0xff;
}

/**
 * Is the given IP in the multicast range ?
 *
 * http://www.iana.org/assignments/multicast-addresses/multicast-addresses.xhtml
 *
 * @param	ip the ip to check, must be in network order (big indian)
 * @return	1 if true, 0 if false
 */
static inline bool is_multicast_ipv4(uint32_t ip)
{
	return (((uint8_t *) &ip)[0] >= 224 && ((uint8_t *) &ip)[0] <= 239);
}

#endif /* _PG_IP_H */
