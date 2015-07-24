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

#ifndef _PACKETS_PACKETS_H_
#define _PACKETS_PACKETS_H_

#include <rte_config.h>
#include <rte_mbuf.h>

#include "utils/errors.h"

#define HASH_ENTRIES		(1024 * 32)
#define HASH_KEY_SIZE		8

/**
 * Made as a macro for performance reason since a function would imply a 10%
 * performance hit and a function in a separate module would not be inlined.
 */
#define dst_key_ptr(pkt)						\
	((struct ether_addr *) RTE_MBUF_METADATA_UINT8_PTR(pkt, 0))

/**
 * Made as a macro for performance reason since a function would imply a 10%
 * performance hit and a function in a separate module would not be inlined.
 */
#define src_key_ptr(pkt)						\
	((struct ether_addr *) RTE_MBUF_METADATA_UINT8_PTR(pkt, HASH_KEY_SIZE))

int packets_pack(struct rte_mbuf **dst,
		 struct rte_mbuf **src,
		 uint64_t pkts_mask);

void packets_incref(struct rte_mbuf **pkts, uint64_t pkts_mask);

void packets_free(struct rte_mbuf **pkts, uint64_t pkts_mask);

void packets_forget(struct rte_mbuf **pkts, uint64_t pkts_mask);

void packets_prefetch(struct rte_mbuf **pkts, uint64_t pkts_mask);

int packets_prepare_hash_keys(struct rte_mbuf **pkts,
			      uint64_t pkts_mask,
			      struct switch_error **errp);

void packets_clear_hash_keys(struct rte_mbuf **pkts, uint64_t pkts_mask);

#endif
