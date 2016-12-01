/* Copyright 2014 Outscale SAS
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

#ifndef _PG_UTILS_MEMPOOL_H
#define _PG_UTILS_MEMPOOL_H

#include <rte_config.h>
#include <rte_mempool.h>

#define PG_NUM_MBUFS 8191
#define PG_MBUF_CACHE_SIZE 250
#define PG_MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)

extern struct rte_mempool *mp;

void pg_alloc_mempool(void);

static inline struct rte_mempool *pg_get_mempool(void)
{
	if (unlikely(!mp))
		pg_alloc_mempool();
	return mp;
}

#endif /* _PG_UTILS_MEMPOOL_H */
