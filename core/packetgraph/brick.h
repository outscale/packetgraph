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

#include <packetgraph/packetgraph.h>
#include <packetgraph/brick-int.h>
#include <packetgraph/utils/errors.h>

void pg_brick_set_max_edges(struct pg_brick *brick,
			   uint16_t west_edges, uint16_t east_edges);

 /* lifecycle */
struct pg_brick *pg_brick_new(const char *name,
			      struct pg_brick_config *config,
			      struct pg_error **errp);
void pg_brick_incref(struct pg_brick *brick);
struct pg_brick *pg_brick_decref(struct pg_brick *brick,
				 struct pg_error **errp);
int pg_brick_reset(struct pg_brick *brick, struct pg_error **errp);

/* testing */
uint32_t pg_brick_links_count_get(struct pg_brick *brick,
				  struct pg_brick *target,
				  struct pg_error **errp);

/* generic functions used to factorize code */

/* these are generic brick_ops callbacks */
int pg_brick_generic_west_link(struct pg_brick *target,
			       struct pg_brick *brick,
			       struct pg_error **errp);
int pg_brick_generic_east_link(struct pg_brick *target,
			       struct pg_brick *brick,
			       struct pg_error **errp);
void pg_brick_generic_unlink(struct pg_brick *brick, struct pg_error **errp);

/* data flow */
int pg_brick_burst(struct pg_brick *brick, enum pg_side from,
		   uint16_t edge_index,
		   struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		   struct pg_error **errp);

/* data flow commodity functions */
int pg_brick_burst_to_east(struct pg_brick *brick, uint16_t edge_index,
			struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
			struct pg_error **errp);

int pg_brick_burst_to_west(struct pg_brick *brick, uint16_t edge_index,
			   struct rte_mbuf **pkts, uint16_t nb,
			   uint64_t pkts_mask, struct pg_error **errp);

/* used for testing */
struct rte_mbuf **pg_brick_west_burst_get(struct pg_brick *brick,
					  uint64_t *pkts_mask,
					  struct pg_error **errp);

struct rte_mbuf **pg_brick_east_burst_get(struct pg_brick *brick,
					  uint64_t *pkts_mask,
					  struct pg_error **errp);

int pg_brick_side_forward(struct pg_brick_side *brick_side, enum pg_side from,
			  struct rte_mbuf **pkts, uint16_t nb,
			  uint64_t pkts_mask, struct pg_error **errp);

char *pg_brick_handle_dup(struct pg_brick *brick, struct pg_error **errp);

#endif
