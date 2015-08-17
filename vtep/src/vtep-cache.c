/* Copyright 2015 Outscale SAS
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
#include <rte_ether.h>
#include <glib.h>
#include <packetgraph/utils/circular-buffer.h>
#include "vtep-cache.h"

#define CACHE_SIZE 8

REGISTER_CB(cache, struct cache_data, CACHE_SIZE)

struct cache *mac_cache_new(void)
{
	struct cache *c = g_new(struct cache, 1);

	if (!c)
		return NULL;
	cb_init0(*c, cache);
	return c;
}

void mac_cache_add(struct cache *cache, uint64_t *mac,
		   struct dest_addresses *data)
{
	cb_get(*cache, cache->start + 1).key = *mac;
	cb_get(*cache, cache->start + 1).data = *data;
	cb_start_incr(*cache);
}

int mac_cache_find(struct cache *cache, uint64_t *mac)
{
	/* the first idx can not be out of bound, so we didn't use
	 * cb_get, because it's slover than a direct call to
	 * cache->buff[cache->start] */
	if (cache->buff[cache->start].key == *mac)
		return 1;
	for (int i = 1; i < CACHE_SIZE; ++i) {
		if (cb_get(*cache, i).key ==  *mac) {
			cb_set_start(*cache, i);
			return 1;
		}
	}
	return 0;
}

struct dest_addresses *mac_cache_get(struct cache *cache,
				     uint64_t *mac)
{
	/* the first idx can not be out of bound, so we didn't use
	 * cb_get, because it's slover than a direct call to
	 * cache->buff[cache->start] */
	if (cache->buff[cache->start].key == *mac)
		return &cache->buff[cache->start].data;
	for (int i = 1; i < CACHE_SIZE; ++i) {
		if (cb_get(*cache, i).key ==  *mac) {
			cb_set_start(*cache, i);
			return &cb_get(*cache, i).data;
		}
	}
	return 0;
}

void mac_cache_destroy(struct cache *cache)
{
	g_free(cache);
}
