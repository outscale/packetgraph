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
#include <rte_ether.h>

#include <glib.h>
#include <string.h>

#include <packetgraph/packets.h>
#include <packetgraph/utils/bitmask.h>

/**
 * This function copy and pack a source packet array into a destination array
 * It uses the packet mask to know which packet are to copy.
 *
 * @param	dst the destination packet array
 * @param	src the source packet array
 * @param	pkts_mask the packing mask
 * @return	the number of packed packets
 */
int packets_pack(struct rte_mbuf **dst,
		 struct rte_mbuf **src,
		 uint64_t pkts_mask)
{
	int i;

	for (i = 0; pkts_mask; i++) {
		uint16_t j;

		low_bit_iterate(pkts_mask, j);

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
void packets_incref(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for (; pkts_mask;) {
		uint16_t i;

		low_bit_iterate(pkts_mask, i);

		rte_mbuf_refcnt_update(pkts[i], 1);
	}
}

/**
 * This function selectively free members of a packet array
 *
 * @param	pkts the packet array
 * @param	pkts_mask mask of packet to free
 */
void packets_free(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for (; pkts_mask;) {
		uint16_t i;

		low_bit_iterate(pkts_mask, i);

		rte_pktmbuf_free(pkts[i]);
	}
}

/**
 * This function selectively set to NULL references of a packet array
 *
 * @param	pkts the packet array
 * @param	pkts_mask mask of packet to forget
 */
void packets_forget(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for (; pkts_mask;) {
		uint16_t i;

		low_bit_iterate(pkts_mask, i);

		pkts[i] = NULL;
	}
}


void packets_prefetch(struct rte_mbuf **pkts,
		      uint64_t pkts_mask)
{
	uint64_t mask;

	for (mask = pkts_mask; mask; ) {
		struct rte_mbuf *pkt;
		uint16_t i;

		low_bit_iterate(mask, i);

		pkt = pkts[i];
		rte_prefetch0(pkt);

	}
	for (mask = pkts_mask; mask; ) {
		struct ether_hdr *eth_hdr;
		struct rte_mbuf *pkt;
		uint16_t i;

		low_bit_iterate(mask, i);

		pkt = pkts[i];
		eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
		rte_prefetch0(eth_hdr);

	}
}

int packets_prepare_hash_keys(struct rte_mbuf **pkts,
			     uint64_t pkts_mask,
			     struct switch_error **errp)
{
	for ( ; pkts_mask; ) {
		struct ether_hdr *eth_hdr;
		struct rte_mbuf *pkt;
		uint16_t i;

		low_bit_iterate(pkts_mask, i);

		pkt = pkts[i];

		eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);

		if (unlikely(pkt->data_off < HASH_KEY_SIZE * 2)) {
			*errp = error_new("Not enough headroom space");
			return 0;
		}

		memset(dst_key_ptr(pkt), 0, HASH_KEY_SIZE * 2);
		ether_addr_copy(&eth_hdr->d_addr, dst_key_ptr(pkt));
		ether_addr_copy(&eth_hdr->s_addr, src_key_ptr(pkt));
		rte_prefetch0(dst_key_ptr(pkt));
	}

	return 1;
}

void packets_clear_hash_keys(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for ( ; pkts_mask; ) {
		struct rte_mbuf *pkt;
		uint16_t i;

		low_bit_iterate(pkts_mask, i);

		pkt = pkts[i];

		g_assert(pkt->data_off >= HASH_KEY_SIZE * 2);

		memset(dst_key_ptr(pkt), 0, HASH_KEY_SIZE * 2);
	}
}
