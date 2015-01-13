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

#include "packets/packets.h"

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
		uint64_t bit;
		uint16_t j;

		j =  __builtin_ctzll(pkts_mask);
		bit = 1LLU << j;
		pkts_mask &= ~bit;

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
		uint64_t bit;
		uint16_t i;

		i =  __builtin_ctzll(pkts_mask);
		bit = 1LLU << i;
		pkts_mask &= ~bit;

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
		uint64_t bit;
		uint16_t i;

		i =  __builtin_ctzll(pkts_mask);
		bit = 1LLU << i;
		pkts_mask &= ~bit;

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
		uint64_t bit;
		uint16_t i;

		i =  __builtin_ctzll(pkts_mask);
		bit = 1LLU << i;
		pkts_mask &= ~bit;

		pkts[i] = NULL;
	}
}
