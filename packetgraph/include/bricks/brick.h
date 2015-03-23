/* Copyright 2014 Nodalink EURL
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

#ifndef _BRICKS_BRICK_H_
#define _BRICKS_BRICK_H_

#include "bricks/brick-int.h"
#include "bricks/brick-api.h"
#include "utils/errors.h"

void brick_set_max_edges(struct brick *brick,
			   uint16_t west_edges, uint16_t east_edges);

 /* lifecycle */
struct brick *brick_new(const char *name, struct brick_config *config,
			 struct switch_error **errp);
void brick_incref(struct brick *brick);
struct brick *brick_decref(struct brick *brick, struct switch_error **errp);
int brick_reset(struct brick *brick, struct switch_error **errp);

/* testing */
uint32_t brick_links_count_get(struct brick *brick,
			   struct brick *target, struct switch_error **errp);

/* generic functions used to factorize code */

/* these are generic brick_ops callbacks */
int brick_generic_west_link(struct brick *target,
			    struct brick *brick, struct switch_error **errp);
int brick_generic_east_link(struct brick *target,
			    struct brick *brick, struct switch_error **errp);
void brick_generic_unlink(struct brick *brick, struct switch_error **errp);

/* data flow */
int brick_burst(struct brick *brick, enum side from, uint16_t edge_index,
		struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		struct switch_error **errp);

/* data flow commodity functions */
int brick_burst_to_east(struct brick *brick, uint16_t edge_index,
			struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
			struct switch_error **errp);

int brick_burst_to_west(struct brick *brick, uint16_t edge_index,
			struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
			struct switch_error **errp);

/* used for testing */
struct rte_mbuf **brick_west_burst_get(struct brick *brick,
				       uint64_t *pkts_mask,
				       struct switch_error **errp);
struct rte_mbuf **brick_east_burst_get(struct brick *brick,
				       uint64_t *pkts_mask,
				       struct switch_error **errp);

int brick_side_forward(struct brick_side *brick_side, enum side from,
		       struct rte_mbuf **pkts, uint16_t nb,
		       uint64_t pkts_mask, struct switch_error **errp);

char *brick_handle_dup(struct brick *brick, struct switch_error **errp);

/* Add for testing purpose SHOULD BE REMOVE(or move) ! */

void vtep_add_vni(struct brick *brick,
		   struct brick *neighbor,
		   uint32_t vni, uint32_t multicast_ip,
		   struct switch_error **errp);

#include <rte_ether.h>

void vtep_add_mac(struct brick *brick,
		   struct brick *neighbor,
		   struct ether_addr *mac,
		   struct switch_error **errp);

#endif
