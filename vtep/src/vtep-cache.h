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

#ifndef _PG_VTEP_VTEP_CACHE_H
#define _PG_VTEP_VTEP_CACHE_H

#include <rte_config.h>
#include <rte_ether.h>

/**
 * hold a couple of destination MAC and IP addresses
 */
struct dest_addresses {
	uint32_t ip;
	struct ether_addr mac;
};

struct cache_data {
	uint64_t key;
	struct dest_addresses data;
};

struct cache;

struct cache *mac_cache_new(void);

void mac_cache_add(struct cache *cache, uint64_t *mac,
		   struct dest_addresses *data);

int mac_cache_find(struct cache *cache, uint64_t *mac);

struct dest_addresses *mac_cache_get(struct cache *cache,
				     uint64_t *mac);

void mac_cache_destroy(struct cache *cache);


#endif /* _PG_VTEP_VTEP_CACHE_H */
