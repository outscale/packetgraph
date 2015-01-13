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

int packets_pack(struct rte_mbuf **dst,
		 struct rte_mbuf **src,
		 uint64_t pkts_mask);

void packets_incref(struct rte_mbuf **pkts, uint64_t pkts_mask);

void packets_free(struct rte_mbuf **pkts, uint64_t pkts_mask);

void packets_forget(struct rte_mbuf **pkts, uint64_t pkts_mask);

#endif
