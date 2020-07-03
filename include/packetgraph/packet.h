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

#ifndef _PG_PACKET_H
#define _PG_PACKET_H

#include <stddef.h>
#include <packetgraph/common.h>

/**
 * This typedef is here so you can handle packets as abstract type,
 * usefull in C++, that tend to break with DPDK includes.
 * But if you want to use struct rte_mbuf in you callback,
 * We guarentee, that we'll keep pg_packet_t as a typdef of
 * struct rte_mbuf
 */
IGNORE_NEW_TYPEDEFS typedef struct rte_mbuf pg_packet_t;

/**
 * Get pointer to packet's data
 *
 * @param   packet the packet
 * @return  a pointer to packet's data.
 *          If you alter packet's content, you may want to set it's size
 *          with pg_packet_len_set.
 */
PG_WARN_UNUSED
uint8_t *pg_packet_data(pg_packet_t *packet);

/**
 * @return a pointer to packet's data cast into a pointer of type
 */
#define pg_packet_cast_data(pkt, type)		\
	((type *)pg_packet_data(pkt))

/**
 * Get packet's length
 *
 * @param   packet the packet
 * @return  len of the packet in bytes
 */
PG_WARN_UNUSED
unsigned int pg_packet_len(pg_packet_t *packet);

/**
 * Get packet's max length
 *
 * @param   packet the packet
 * @return  maximum len of the packet in bytes
 */
PG_WARN_UNUSED
unsigned int pg_packet_max_len(pg_packet_t *packet);

/**
 * Compute packet len base on packet layers len + l5_len
 * @param   packet packet's pointer
 * @param   l5_len length to adition to other l*_len, see pg_packet_set_l*_len
 * @return  -1 of len exceeds packet's max size, 0 otherwise
 */
PG_WARN_UNUSED
int pg_packet_compute_len(pg_packet_t *packet, unsigned int l5_len);

/**
 * Set packet's len
 *
 * @param   packet packet's pointer
 * @param   len new packet's length
 * @return  -1 of len exceeds packet's max size, 0 otherwise
 */
int pg_packet_set_len(pg_packet_t *packet, unsigned int len);

void pg_packet_set_l2_len(pg_packet_t *packet, unsigned int len);
int pg_packet_get_l2_len(pg_packet_t *packet);
void pg_packet_set_l3_len(pg_packet_t *packet, unsigned int len);
int pg_packet_get_l3_len(pg_packet_t *packet);
void pg_packet_set_l4_len(pg_packet_t *packet, unsigned int len);
int pg_packet_get_l4_len(pg_packet_t *packet);

/**
 * iterate on each packets in 'pkts' base on 'mask'
 * @param   pkts an array packets
 * @param   mask packet mask
 * @param   pkt the current packet
 * @param   it the current pos
 */
#define PG_PACKETS_FOREACH(pkts, mask, pkt, it)				\
	pg_packet_t *pkt;						\
	for (uint64_t tmpmask = (mask), it = 0;				\
	     tmpmask && ((it = __builtin_ctzll(tmpmask)) || 1)		\
		     && (pkt = (pkts)[it]);				\
	     tmpmask &= ~(1LLU << it))


/**
 * Compute checksum of an ipv4 header
 *
 * Note: you should set your checksum to 0.
 * @param   ip_hdr pointer to ip's header
 * @return  checksum value
 */
uint16_t pg_packet_ipv4_checksum(uint8_t *ip_hdr);

uint16_t pg_packet_sum(uint8_t *ip_hdr, size_t len);

#endif  /* _PG_PACKET_H */
