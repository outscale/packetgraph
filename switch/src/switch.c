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

#include <rte_config.h>
#include <rte_errno.h>
#include <rte_ether.h>
#include <rte_hash_crc.h>
#include <rte_memcpy.h>
#include <rte_table.h>
#include <rte_table_hash.h>
#include <rte_prefetch.h>

#define HASH_ENTRIES		(1024 * 32)
#define HASH_KEY_SIZE		8

#include <packetgraph/brick.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/common.h>
#include <packetgraph/switch.h>
#include <packetgraph/packets.h>
#include <packetgraph/utils/mac-table.h>

/* this tell from where a given source mac address came */
struct pg_address_source {
	uint16_t edge_index;    /* index of the port the packet came from */
	enum pg_side from;	/* side of the switch the packet came from */
};

/* structure used to describe a side (EAST_SIDE/WEST_SIDE) of the switch */
struct pg_switch_side {
	struct pg_address_source *sources;
	uint64_t *masks;	/* outgoing packet masks (one per port) */
};

struct pg_switch_state {
	struct pg_brick brick;
	struct pg_mac_table table;
	enum pg_side output;
	struct pg_switch_side sides[MAX_SIDE];	/* sides of the switch */
};

struct pg_switch_config {
	enum pg_side output;
};

static inline uint64_t ether_hash(void *key, uint32_t key_size, uint64_t seed)
{
	return _mm_crc32_u64(seed, *((uint64_t *) key));
}

static inline void flood(struct pg_switch_state *state,
			 struct pg_address_source *source,
			 uint64_t mask)
{
	struct pg_switch_side *switch_side;
	enum pg_side i;
	uint16_t j;

	if (!mask)
		return;

	for (i = 0; i < MAX_SIDE; i++) {
		switch_side = &state->sides[i];
		for (j = 0; j < state->brick.sides[i].max; j++)
			switch_side->masks[j] |= mask;
	}
}

static inline void zero_masks(struct pg_switch_state *state)
{
	enum pg_side i;

	for (i = 0; i < MAX_SIDE; i++)
		memset(state->sides[i].masks, 0,
		       state->brick.sides[i].max * sizeof(uint64_t));
}

static inline int forward(struct pg_switch_state *state, enum pg_side to,
			  uint16_t index, struct rte_mbuf **pkts,
			  struct pg_error **errp)
{
	struct pg_brick_edge *edge =  &state->brick.sides[to].edges[index];
	struct pg_switch_side *switch_side = &state->sides[to];
	uint64_t mask;

	if (!edge->link)
		return 0;

	mask = switch_side->masks[index];

	if (!mask)
		return 0;


	switch_side->masks[index] = 0;
	return pg_brick_burst(edge->link, pg_flip_side(to), edge->pair_index,
			      pkts, mask, errp);
}

static int forward_bursts(struct pg_switch_state *state,
			  struct pg_address_source *source,
			  struct rte_mbuf **pkts,
			  struct pg_error **errp)
{
	struct pg_switch_side *switch_side = &state->sides[source->from];
	enum pg_side i = state->output;
	uint16_t j;
	int ret;

	/* never forward on source port */
	switch_side->masks[source->edge_index] = 0;

	for (j = 0; j < state->brick.sides[i].max; j++) {
		ret = forward(state, i, j, pkts, errp);

		if (ret < 0)
			return -1;
	}

	i = pg_flip_side(i);
	for (j = 0; j < state->brick.sides[i].max; j++) {
		ret = forward(state, i, j, pkts, errp);

		if (ret < 0)
			return -1;
	}

	return 0;
}

static inline bool is_filtered(struct ether_addr *eth_addr)
{
	return eth_addr->addr_bytes[0] == 0x01 &&
		eth_addr->addr_bytes[1] == 0x80 &&
		eth_addr->addr_bytes[2] == 0xC2 &&
		eth_addr->addr_bytes[3] == 0x00 &&
		eth_addr->addr_bytes[4] == 0x00 &&
		eth_addr->addr_bytes[5] <= 0x0F;
}

static inline void learn_addr(struct pg_switch_state *state,
			     uint8_t *key,
			     struct pg_address_source *source)
{
	pg_mac_table_ptr_set(&state->table, *((union pg_mac *)key),
			     source);
}

static void do_learn_filter_multicast(struct pg_switch_state *state,
				     struct pg_address_source *source,
				     struct rte_mbuf **pkts,
				     uint64_t pkts_mask,
				     uint64_t *unicast_mask,
				     struct pg_error **errp)
{
	uint64_t filtered_mask = 0, flood_mask = 0, mask;

	for (mask = pkts_mask; mask;) {
		struct ether_hdr *eth_hdr;
		struct rte_mbuf *pkt;
		uint64_t bit;
		uint16_t i;

		pg_low_bit_iterate_full(mask, bit, i);

		pkt = pkts[i];

		eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);

		/* associate source mac address with its source port */
		learn_addr(state, (void *) &eth_hdr->s_addr,
			   source);

		/* http://standards.ieee.org/regauth/groupmac/tutorial.html */
		if (unlikely(is_filtered(&eth_hdr->d_addr))) {
			filtered_mask |= bit;
			continue;
		}

		/* if the packet is multicast or broadcast flood it */
		if (unlikely(is_multicast_ether_addr(&eth_hdr->d_addr))) {
			flood_mask |= bit;
			continue;
		}

	}

	flood(state, source, flood_mask);

	*unicast_mask = pkts_mask & ~filtered_mask & ~flood_mask;
}

static void do_switch(struct pg_switch_state *state,
		      struct pg_address_source *source,
		      struct rte_mbuf **pkts,
		      uint64_t pkts_mask)
{
	uint64_t flood_mask = 0;
	struct ether_hdr *eth_hdr;

	for ( ; pkts_mask; ) {
		struct pg_switch_side *switch_side;
		struct pg_address_source *entry;
		uint16_t edge_index;
		uint64_t bit;
		uint16_t i;

		pg_low_bit_iterate_full(pkts_mask, bit, i);

		eth_hdr = rte_pktmbuf_mtod(pkts[i], struct ether_hdr *);
		entry = pg_mac_table_ptr_get(
			&state->table,
			*((union pg_mac *)&eth_hdr->d_addr));
		if (entry) {
			switch_side = &state->sides[entry->from];
			edge_index = entry->edge_index;

		/* the lookup table entry is stale due to a port hotplug */
		} else {
			flood_mask |= bit;
			continue;
		}

		/* mark the packet for forwarding on the correct port */
		switch_side->masks[edge_index] |= bit;
	}

	flood(state, source, flood_mask);
}

static int switch_burst(struct pg_brick *brick, enum pg_side from,
			uint16_t edge_index, struct rte_mbuf **pkts,
			uint64_t pkts_mask,
			struct pg_error **errp)
{
	struct pg_switch_state *state =
		pg_brick_get_state(brick, struct pg_switch_state);
	uint64_t unicast_mask = 0;
	struct pg_address_source *source;

	source = &state->sides[from].sources[edge_index];
	source->from = from;
	source->edge_index = edge_index;

	do_learn_filter_multicast(state, source, pkts,
				  pkts_mask, &unicast_mask, errp);

	do_switch(state, source, pkts, unicast_mask);

	return forward_bursts(state, source, pkts, errp);
}

static int switch_init(struct pg_brick *brick,
		       struct pg_brick_config *config, struct pg_error **errp)
{
	struct pg_switch_state *state =
		pg_brick_get_state(brick, struct pg_switch_state);
	enum pg_side i;

	if (!config->brick_config) {
		*errp = pg_error_new("config->brick_config is NULL");
		return 0;
	}

	brick->burst = switch_burst;

	if (pg_mac_table_init(&state->table) < 0) {
		*errp = pg_error_new(
				"Failed to create hash for switch brick '%s'",
				brick->name);
		return 0;
	}

	for (i = 0; i < MAX_SIDE; i++) {
		uint16_t max = brick->sides[i].max;

		state->sides[i].masks	= g_new0(uint64_t, max);
		state->sides[i].sources	= g_new0(struct pg_address_source, max);
	}
	zero_masks(state);
	state->output =
	  ((struct pg_switch_config *)config->brick_config)->output;
	return 1;
}

static struct pg_brick_config *pg_switch_config_new(const char *name,
						    uint32_t west_max,
						    uint32_t east_max,
						    enum pg_side output)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_switch_config *switch_config = g_new0(struct pg_switch_config,
							1);
	switch_config->output = output;
	config->brick_config = switch_config;
	return  pg_brick_config_init(config, name, west_max,
				     east_max, PG_MULTIPOLE);
}

struct pg_brick *pg_switch_new(const char *name,
			       uint32_t west_max,
			       uint32_t east_max,
			       enum pg_side output,
			       struct pg_error **errp)
{
	struct pg_brick_config *config = pg_switch_config_new(name, west_max,
							     east_max, output);
	struct pg_brick *ret = pg_brick_new("switch", config, errp);

	pg_brick_config_free(config);
	return ret;
}


static void switch_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_switch_state *state =
		pg_brick_get_state(brick, struct pg_switch_state);
	enum pg_side i;

	for (i = 0; i < MAX_SIDE; i++) {
		g_free(state->sides[i].masks);
		g_free(state->sides[i].sources);
	}

	pg_mac_table_free(&state->table);
}

static void switch_unlink_notify(struct pg_brick *brick,
				 enum pg_side side, uint16_t edge_index,
				 struct pg_error **errp)
{
	struct pg_switch_state *state =
		pg_brick_get_state(brick, struct pg_switch_state);

	PG_MAC_TABLE_FOREACH(&state->table, cur_mac,
			     struct pg_address_source *, src) {
		if (likely(src != NULL) &&
		    src->from == side &&
		    src->edge_index == edge_index)
			pg_mac_table_ptr_unset(&state->table, cur_mac);
	}
}

static struct pg_brick_ops switch_ops = {
	.name		= "switch",
	.state_size	= sizeof(struct pg_switch_state),

	.init		= switch_init,
	.destroy	= switch_destroy,

	.unlink		= pg_brick_generic_unlink,

	.unlink_notify  = switch_unlink_notify,
};

pg_brick_register(switch, &switch_ops);
