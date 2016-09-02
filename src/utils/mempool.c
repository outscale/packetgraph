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

#include <glib.h>
#include <rte_config.h>
#include <rte_mbuf.h>
#include "utils/mempool.h"

static struct rte_mempool *mp;

static void pg_alloc_mempool(void)
{
	mp = rte_mempool_create("pg_mempool", PG_NUM_MBUFS, PG_MBUF_SIZE,
				PG_MBUF_CACHE_SIZE,
				sizeof(struct rte_pktmbuf_pool_private),
				rte_pktmbuf_pool_init, NULL,
				rte_pktmbuf_init, NULL,
				rte_socket_id(), 0);
	g_assert(mp);
}

struct rte_mempool *pg_get_mempool(void)
{
	if (!mp)
		pg_alloc_mempool();
	return mp;
}
