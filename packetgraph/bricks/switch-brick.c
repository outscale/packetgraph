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

#include "bricks/brick.h"
#include "utils/bitmask.h"
#include "common.h"

/* this tell from where a given source mac address came */
struct address_source {
	uint64_t unlink;	/* hot plugging unlink */
	uint16_t edge_index;    /* index of the port the packet came from */
	enum side from;		/* side of the switch the packet came from */
};

/* structure used to describe a side (EAST_SIDE/WEST_SIDE) of the switch */
struct switch_side {
	uint64_t *masks;	/* outgoing packet masks (one per port) */
	uint64_t *unlinks;	/* hot plugging unlink  (one per port) */
};

struct switch_state {
	struct brick brick;
	void *table;				/* mac addy lookup table */
	struct switch_side sides[MAX_SIDE];	/* sides of the switch */
};

static inline uint64_t ether_hash(void *key, uint32_t key_size, uint64_t seed)
{
	return _mm_crc32_u64(seed, *((uint64_t *) key));
}

static inline void flood(struct switch_state *state,
			 struct address_source *source,
			 uint64_t mask)
{
	struct switch_side *switch_side;
	enum side i;
	uint16_t j;

	if (!mask)
		return;

	for (i = 0; i < MAX_SIDE; i++) {
		switch_side = &state->sides[i];
		for (j = 0; j < state->brick.sides[i].max; j++)
			switch_side->masks[j] |= mask;
	}
}

static inline void zero_masks(struct switch_state *state)
{
	enum side i;

	for (i = 0; i < MAX_SIDE; i++)
		memset(state->sides[i].masks, 0,
			state->brick.sides[i].max * sizeof(uint64_t));
}

static inline int forward(struct switch_state *state, enum side to,
			  uint16_t index, struct rte_mbuf **pkts, uint16_t nb,
			  struct switch_error **errp)
{
	struct brick_edge *edge =  &state->brick.sides[to].edges[index];
	struct switch_side *switch_side = &state->sides[to];

	if (!switch_side->masks[index])
		return 1;

	if (!edge->link)
		return 1;

	return brick_burst(edge->link, flip_side(to), edge->pair_index,
			   pkts, nb, switch_side->masks[index], errp);
}

static int forward_bursts(struct switch_state *state,
			  struct address_source *source,
			  struct rte_mbuf **pkts, uint16_t nb,
			  struct switch_error **errp)
{
	struct switch_side *switch_side = &state->sides[source->from];
	enum side i;
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

static inline int learn_addr(struct switch_state *state,
			     uint8_t *key,
			     struct address_source *source,
			     struct switch_error **errp)
{
	void *entry = NULL;
	int key_found;
	int ret;

	/* memorize from where this address_source mac address come from */
	ret = rte_table_hash_key8_lru_dosig_ops.f_add(state->table, key, source,
						 &key_found, &entry);
	if (unlikely(ret))
		*errp = error_new_errno(-ret, "Fail to learn source address");

	return !ret;
}

static inline struct ether_addr *dst_key_ptr(struct rte_mbuf *pkt)
{
	return (struct ether_addr *) RTE_MBUF_METADATA_UINT8_PTR(pkt, 0);
}

static inline struct ether_addr *src_key_ptr(struct rte_mbuf *pkt)
{
	return (struct ether_addr *) RTE_MBUF_METADATA_UINT8_PTR(pkt,
								 HASH_KEY_SIZE);
}

static int do_learn_filter_multicast(struct switch_state *state,
				     struct address_source *source,
				     struct rte_mbuf **pkts,
				     uint64_t pkts_mask,
				     uint64_t *unicast_mask,
				     struct switch_error **errp)
{
	uint64_t filtered_mask = 0, flood_mask = 0, mask;

	for (mask = pkts_mask; mask;) {
		struct ether_hdr *eth_hdr;
		struct rte_mbuf *pkt;
		uint64_t bit;
		uint16_t i;
		int ret;

		low_bit_iterate_full(mask, bit, i);

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

static void do_switch(struct switch_state *state,
		      struct address_source *source,
		      uint64_t pkts_mask,
		      uint64_t lookup_hit_mask,
		      struct address_source **entries)
{
	uint64_t flood_mask = pkts_mask & ~lookup_hit_mask;

	for ( ; lookup_hit_mask; ) {
		struct switch_side *switch_side;
		struct address_source *entry;
		uint16_t edge_index;
		enum side from;
		uint64_t bit;
		uint16_t i;

		low_bit_iterate_full(lookup_hit_mask, bit, i);

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

static void prefetch_packets(struct rte_mbuf **pkts,
			     uint64_t pkts_mask)
{
	uint64_t mask;

	for (mask = pkts_mask; mask; ) {
		struct rte_mbuf *pkt;
		uint16_t i;

		low_bit_iterate(mask, i);

		pkt = pkts[i];
		rte_prefetch0(pkt);

	}
	for (mask = pkts_mask; mask; ) {
		struct ether_hdr *eth_hdr;
		struct rte_mbuf *pkt;
		uint16_t i;

		low_bit_iterate(mask, i);

		pkt = pkts[i];
		eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
		rte_prefetch0(eth_hdr);

	}
}

static int prepare_hash_keys(struct rte_mbuf **pkts,
			     uint64_t pkts_mask,
			     struct switch_error **errp)
{
	for ( ; pkts_mask; ) {
		struct ether_hdr *eth_hdr;
		struct rte_mbuf *pkt;
		uint16_t i;

		low_bit_iterate(pkts_mask, i);

		pkt = pkts[i];

		eth_hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);

		if (unlikely(pkt->data_off < HASH_KEY_SIZE * 2)) {
			*errp = error_new("Not enough headroom space");
			return 0;
		}

		memset(dst_key_ptr(pkt), 0, HASH_KEY_SIZE * 2);
		ether_addr_copy(&eth_hdr->d_addr, dst_key_ptr(pkt));
		ether_addr_copy(&eth_hdr->s_addr, src_key_ptr(pkt));
		rte_prefetch0(dst_key_ptr(pkt));
	}

	return 1;
}

static void clear_hash_keys(struct rte_mbuf **pkts, uint64_t pkts_mask)
{
	for ( ; pkts_mask; ) {
		struct rte_mbuf *pkt;
		uint16_t i;

		low_bit_iterate(pkts_mask, i);

		pkt = pkts[i];

		g_assert(pkt->data_off >= HASH_KEY_SIZE * 2);

		memset(dst_key_ptr(pkt), 0, HASH_KEY_SIZE * 2);
	}
}

static int switch_burst(struct brick *brick, enum side from,
			uint16_t edge_index, struct rte_mbuf **pkts,
			uint16_t nb, uint64_t pkts_mask,
			struct switch_error **errp)
{
	struct switch_state *state =
		brick_get_state(brick, struct switch_state);
	uint64_t unicast_mask = 0, lookup_hit_mask = 0;
	struct address_source *entries[64];
	struct address_source source;
	int ret;

	source.from = from;
	source.edge_index = edge_index;
	source.unlink = state->sides[from].unlinks[edge_index];

	prefetch_packets(pkts, pkts_mask);

	zero_masks(state);

	ret = prepare_hash_keys(pkts, pkts_mask, errp);
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
		*errp = error_new_errno(-ret, "Fail to lookup dest address");
		goto no_forward;
	}

	do_switch(state, &source, unicast_mask, lookup_hit_mask,
		  (struct address_source **) entries);

	clear_hash_keys(pkts, pkts_mask);
	return forward_bursts(state, &source, pkts, nb, errp);
no_forward:
	clear_hash_keys(pkts, pkts_mask);
	return 0;
}

static int switch_init(struct brick *brick,
		       struct brick_config *config, struct switch_error **errp)
{
	struct switch_state *state =
		brick_get_state(brick, struct switch_state);
	enum side i;

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
		sizeof(struct address_source));

	if (!state->table) {
		*errp = error_new("Failed to create hash for switch brick '%s'",
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

struct brick *switch_new(const char *name, uint32_t west_max,
			uint32_t east_max,
			struct switch_error **errp)
{
	struct brick_config *config = brick_config_new(name, west_max,
						       east_max);
	struct brick *ret = brick_new("switch", config, errp);

	brick_config_free(config);
	return ret;
}


static void switch_destroy(struct brick *brick, struct switch_error **errp)
{
	struct switch_state *state =
		brick_get_state(brick, struct switch_state);
	enum side i;

	for (i = 0; i < MAX_SIDE; i++) {
		g_free(state->sides[i].masks);
		g_free(state->sides[i].unlinks);
	}

	rte_table_hash_key8_lru_dosig_ops.f_free(state->table);
}

static void switch_unlink_notify(struct brick *brick,
				 enum side side, uint16_t edge_index)
{
	struct switch_state *state =
		brick_get_state(brick, struct switch_state);
	state->sides[side].unlinks[edge_index]++;
}

static struct brick_ops switch_ops = {
	.name		= "switch",
	.state_size	= sizeof(struct switch_state),

	.init		= switch_init,
	.destroy	= switch_destroy,

	.unlink		= brick_generic_unlink,

	.unlink_notify  = switch_unlink_notify,
};

brick_register(struct switch_state, &switch_ops);
