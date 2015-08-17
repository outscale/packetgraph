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

/* this tell from where a given source mac address came */
struct pg_address_source {
	uint64_t unlink;	/* hot plugging unlink */
	uint16_t edge_index;    /* index of the port the packet came from */
	enum pg_side from;	/* side of the switch the packet came from */
};

/* structure used to describe a side (EAST_SIDE/WEST_SIDE) of the switch */
struct pg_switch_side {
	uint64_t *masks;	/* outgoing packet masks (one per port) */
	uint64_t *unlinks;	/* hot plugging unlink  (one per port) */
};

struct pg_switch_state {
	struct pg_brick brick;
	void *table;				/* mac addy lookup table */
	struct pg_switch_side sides[MAX_SIDE];	/* sides of the switch */
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
			  uint16_t index, struct rte_mbuf **pkts, uint16_t nb,
			  struct pg_error **errp)
{
	struct pg_brick_edge *edge =  &state->brick.sides[to].edges[index];
	struct pg_switch_side *switch_side = &state->sides[to];

	if (!switch_side->masks[index])
		return 1;

	if (!edge->link)
		return 1;

	return pg_brick_burst(edge->link, pg_flip_side(to), edge->pair_index,
			      pkts, nb, switch_side->masks[index], errp);
}

static int forward_bursts(struct pg_switch_state *state,
			  struct pg_address_source *source,
			  struct rte_mbuf **pkts, uint16_t nb,
			  struct pg_error **errp)
{
	struct pg_switch_side *switch_side = &state->sides[source->from];
	enum pg_side i;
	uint16_t j;
	int ret;

	/* never forward on source port */
	switch_side->masks[source->edge_index] = 0;

	for (i = 0; i < MAX_SIDE; i++)
		for (j = 0; j < state->brick.sides[i].max; j++) {
			ret = forward(state, i, j, pkts, nb, errp);

			if (!ret)
				return 0;
		}

	return 1;
}

static inline int is_filtered(struct ether_addr *eth_addr)
{
	return eth_addr->addr_bytes[0] == 0x01 &&
		eth_addr->addr_bytes[1] == 0x80 &&
		eth_addr->addr_bytes[2] == 0xC2 &&
		eth_addr->addr_bytes[3] == 0x00 &&
		eth_addr->addr_bytes[4] == 0x00 &&
		eth_addr->addr_bytes[5] <= 0x0F;
}

static inline int learn_addr(struct pg_switch_state *state,
			     uint8_t *key,
			     struct pg_address_source *source,
			     struct pg_error **errp)
{
	void *entry = NULL;
	int key_found;
	int ret;

	/* memorize from where this address_source mac address come from */
	ret = rte_table_hash_key8_lru_dosig_ops.f_add(state->table, key, source,
						      &key_found, &entry);
	if (unlikely(ret))
		*errp = pg_error_new_errno(-ret,
					   "Fail to learn source address");

	return !ret;
}

static int do_learn_filter_multicast(struct pg_switch_state *state,
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
		int ret;

		pg_low_bit_iterate_full(mask, bit, i);

		pkt = pkts[i];

		/* associate source mac address with its source port */
		ret = learn_addr(state, (void *) src_key_ptr(pkt),
				 source, errp);

		if (unlikely(!ret))
			return 0;

		eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);

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
	return 1;
}

static void do_switch(struct pg_switch_state *state,
		      struct pg_address_source *source,
		      uint64_t pkts_mask,
		      uint64_t lookup_hit_mask,
		      struct pg_address_source **entries)
{
	uint64_t flood_mask = pkts_mask & ~lookup_hit_mask;

	for ( ; lookup_hit_mask; ) {
		struct pg_switch_side *switch_side;
		struct pg_address_source *entry;
		uint16_t edge_index;
		enum pg_side from;
		uint64_t bit;
		uint16_t i;

		pg_low_bit_iterate_full(lookup_hit_mask, bit, i);

		entry = entries[i];

		from = entry->from;
		switch_side = &state->sides[from];
		edge_index = entry->edge_index;

		/* the lookup table entry is stale due to a port hotplug */
		if (unlikely(entry->unlink <
			     switch_side->unlinks[edge_index])) {
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
			uint16_t nb, uint64_t pkts_mask,
			struct pg_error **errp)
{
	struct pg_switch_state *state =
		pg_brick_get_state(brick, struct pg_switch_state);
	uint64_t unicast_mask = 0, lookup_hit_mask = 0;
	struct pg_address_source *entries[64];
	struct pg_address_source source;
	int ret;

	source.from = from;
	source.edge_index = edge_index;
	source.unlink = state->sides[from].unlinks[edge_index];

	pg_packets_prefetch(pkts, pkts_mask);

	zero_masks(state);

	ret = pg_packets_prepare_hash_keys(pkts, pkts_mask, errp);
	if (unlikely(!ret))
		return 0;

	ret = do_learn_filter_multicast(state, &source, pkts,
					pkts_mask, &unicast_mask, errp);
	if (unlikely(!ret))
		goto no_forward;

	ret = rte_table_hash_key8_lru_dosig_ops.f_lookup(state->table, pkts,
							 unicast_mask,
							 &lookup_hit_mask,
							 (void **) entries);

	if (unlikely(ret)) {
		*errp = pg_error_new_errno(-ret, "Fail to lookup dest address");
		goto no_forward;
	}

	do_switch(state, &source, unicast_mask, lookup_hit_mask,
		  (struct pg_address_source **) entries);

	pg_packets_clear_hash_keys(pkts, pkts_mask);
	return forward_bursts(state, &source, pkts, nb, errp);
no_forward:
	pg_packets_clear_hash_keys(pkts, pkts_mask);
	return 0;
}

static int switch_init(struct pg_brick *brick,
		       struct pg_brick_config *config, struct pg_error **errp)
{
	struct pg_switch_state *state =
		pg_brick_get_state(brick, struct pg_switch_state);
	enum pg_side i;

	struct rte_table_hash_key8_lru_params hash_params = {
		.n_entries		= HASH_ENTRIES,
		.f_hash			= ether_hash,
		.seed			= 0,
		.signature_offset	= 0,
		.key_offset		= 0,
	};

	brick->burst = switch_burst;

	state->table = rte_table_hash_key8_lru_dosig_ops.f_create(&hash_params,
			rte_socket_id(),
			sizeof(struct pg_address_source));

	if (!state->table) {
		*errp = pg_error_new(
				"Failed to create hash for switch brick '%s'",
				brick->name);
		return 0;
	}

	for (i = 0; i < MAX_SIDE; i++) {
		uint16_t max = brick->sides[i].max;

		state->sides[i].masks	= g_new0(uint64_t, max);
		state->sides[i].unlinks = g_new0(uint64_t, max);
	}

	return 1;
}

struct pg_brick *pg_switch_new(const char *name,
			       uint32_t west_max,
			       uint32_t east_max,
			       struct pg_error **errp)
{
	struct pg_brick_config *config = pg_brick_config_new(name, west_max,
							     east_max);
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
		g_free(state->sides[i].unlinks);
	}

	rte_table_hash_key8_lru_dosig_ops.f_free(state->table);
}

static void switch_unlink_notify(struct pg_brick *brick,
				 enum pg_side side, uint16_t edge_index,
				 struct pg_error **errp)
{
	struct pg_switch_state *state =
		pg_brick_get_state(brick, struct pg_switch_state);
	state->sides[side].unlinks[edge_index]++;
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
