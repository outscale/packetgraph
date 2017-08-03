/* Copyright 2014 Outscale SAS
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

#ifndef _PG_UTILS_MEMPOOL_H
#define _PG_UTILS_MEMPOOL_H

#include <rte_config.h>
#include <rte_mempool.h>

#define PG_NUM_MBUFS 8191
#define PG_MBUF_CACHE_SIZE 250
#define PG_MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

extern struct rte_mempool *mp;

/**
 * @param flags mempool flags
 */
void pg_alloc_mempool(uint32_t flags);

static inline struct rte_mempool *pg_get_mempool(void)
{
	return mp;
}

#endif /* _PG_UTILS_MEMPOOL_H */
