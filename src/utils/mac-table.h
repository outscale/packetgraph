/* Copyright 2016 Outscale SAS
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

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "utils/mac.h"
#include "utils/bitmask.h"

#ifndef PG_UTILS_MAC_TABLE_H
#define PG_UTILS_MAC_TABLE_H

#define PG_MAC_TABLE_MASK_SIZE (0xffffff / 64 + 1)

struct pg_mac_table_ptr {
	uint64_t mask[PG_MAC_TABLE_MASK_SIZE];
	void **entries;
};

struct pg_mac_table_elem {
	uint64_t mask[PG_MAC_TABLE_MASK_SIZE];
	int8_t *entries;
};


/**
 * A mac array contening a ptr
 */
struct pg_mac_table {
	uint64_t mask[PG_MAC_TABLE_MASK_SIZE];
	union {
		struct pg_mac_table_ptr **ptrs;
		struct pg_mac_table_elem **elems;
	};
};

struct pg_mac_table_iterator {
	struct pg_mac_table *table;
	union pg_mac key;
	int is_end;
};

#define pg_mac_table_mask_idx(mac_p) ((mac_p) / 64)
#define pg_mac_table_mask_pos(mac_p) ((mac_p) & 63)

#define pg_mac_table_is_set(ma, mac_p)			\
	(!!((ma).mask[pg_mac_table_mask_idx(mac_p)] &	\
	    (ONE64 << pg_mac_table_mask_pos(mac_p))))

#define pg_mac_table_mask_set(ma, mac_p)		\
	((ma).mask[pg_mac_table_mask_idx(mac_p)] |=	\
	 (ONE64 << pg_mac_table_mask_pos(mac_p)))

#define pg_mac_table_mask_unset(ma, mac_p)		\
	((ma).mask[pg_mac_table_mask_idx(mac_p)] ^=	\
	 (ONE64 << pg_mac_table_mask_pos(mac_p)))


static inline int pg_mac_table_init(struct pg_mac_table *ma)
{
	ma->ptrs = malloc((0xffffff + 1) * sizeof(struct pg_mac_table_ptr *));
	if (!ma->ptrs)
		return -1;
	memset(ma->mask, 0, (PG_MAC_TABLE_MASK_SIZE) * sizeof(uint64_t));
	return 0;
}

static inline void pg_mac_table_free(struct pg_mac_table *ma)
{
	if (!ma)
		return;
	for (int i = 0; i < (PG_MAC_TABLE_MASK_SIZE); ++i) {
		if (!ma->mask[i])
			continue;
		PG_FOREACH_BIT(ma->mask[i], it) {
			struct pg_mac_table_ptr *cur = ma->ptrs[i * 64 + it];

			free(cur->entries);
			free(cur);
		}
	}
	free(ma->ptrs);
}

#define pg_mac_table_part2(mac) (mac.part2 & 0x00ffffff)
#define pg_mac_table_part1(mac) (mac.bytes32[0] & 0x00ffffff)

static inline void pg_mac_table_elem_set(struct pg_mac_table *ma,
					 union pg_mac mac, void *entry,
					 size_t elem_size)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	/* Part 1 is unset */
	if (unlikely(!pg_mac_table_is_set(*ma, part1))) {
		pg_mac_table_mask_set(*ma, part1);
		ma->elems[part1] = malloc(sizeof(struct pg_mac_table_elem));
		g_assert(ma->elems[part1]);
		memset(ma->ptrs[part1]->mask, 0,
		       (PG_MAC_TABLE_MASK_SIZE) * sizeof(uint64_t));
		ma->elems[part1]->entries = malloc((0xffffff + 1) * elem_size);
	}

	if (pg_mac_table_is_set(*ma->elems[part1], part2))
		return;

	pg_mac_table_mask_set(*ma->elems[part1], part2);
	memcpy(ma->elems[part1]->entries + (part2 * elem_size),
	       entry, elem_size);
}

static inline void pg_mac_table_ptr_set(struct pg_mac_table *ma,
					union pg_mac mac, void *entry)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	/* Part 1 is unset */
	if (unlikely(!pg_mac_table_is_set(*ma, part1))) {
		pg_mac_table_mask_set(*ma, part1);
		ma->ptrs[part1] = malloc(sizeof(struct pg_mac_table_ptr));
		g_assert(ma->ptrs[part1]);
		memset(ma->ptrs[part1]->mask, 0,
		       (PG_MAC_TABLE_MASK_SIZE) * sizeof(uint64_t));
		ma->ptrs[part1]->entries =
			malloc((0xffffff + 1) * sizeof(void *));
	}

	/*
	 * Admiting Part 1 is set
	 * Set part 2 regarless if it was set before
	 */
	pg_mac_table_mask_set(*ma->ptrs[part1], part2);
	ma->ptrs[part1]->entries[part2] = entry;
}

static inline void pg_mac_table_ptr_unset(struct pg_mac_table *ma,
					  union pg_mac mac)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	if (!pg_mac_table_is_set(*ma, part1))
		return;
	if (!pg_mac_table_is_set(*ma->ptrs[part1], part2))
		return;
	pg_mac_table_mask_unset(*ma->ptrs[part1], part2);
	if (!ma->ptrs[part1]->mask) {
		free(ma->ptrs[part1]->entries);
		free(ma->ptrs[part1]);
		pg_mac_table_mask_unset(*ma, part1);
	}
}

#define pg_mac_table_elem_get(ma, mac, elem_type)			\
	((elem_type *)pg_mac_table_elem_get_internal(ma, mac,		\
						     sizeof(elem_type)))

static inline void *pg_mac_table_elem_get_internal(struct pg_mac_table *ma,
						   union pg_mac mac,
						   size_t elem_size)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	/* Part 1 is unset */
	if (unlikely(!pg_mac_table_is_set(*ma, part1)))
		return NULL;
	if (unlikely(!pg_mac_table_is_set(*ma->ptrs[part1], part2)))
		return NULL;
	return ma->elems[part1]->entries + (part2 * elem_size);
}

static inline void *pg_mac_table_ptr_get(struct pg_mac_table *ma,
					 union pg_mac mac)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	/* Part 1 is unset */
	if (unlikely(!pg_mac_table_is_set(*ma, part1)))
		return NULL;
	if (unlikely(!pg_mac_table_is_set(*ma->ptrs[part1], part2)))
		return NULL;
	return ma->ptrs[part1]->entries[part2];
}

static inline void pg_mac_table_iterator_next(struct pg_mac_table_iterator *it)
{
	struct pg_mac_table *ma = it->table;
	uint32_t part1 = pg_mac_table_part1(it->key);
	uint32_t part2 = pg_mac_table_part2(it->key);
	int first_mask = pg_mac_table_mask_idx(part1);
	int first_sub_mask = pg_mac_table_mask_idx(part2);
	uint64_t *masks = ma->mask;
	uint64_t unset;
	uint64_t sub_unset;

	unset = ~((ONE64 << (pg_mac_table_mask_pos(part1) + 1)) - ONE64);
	sub_unset = ~((ONE64 << (pg_mac_table_mask_pos(part2) + 1)) - ONE64);

	for (int i = first_mask; i < (PG_MAC_TABLE_MASK_SIZE); ++i) {
		if (!masks[i])
			continue;

		PG_FOREACH_BIT(ma->mask[i] & unset, i2) {
			uint64_t *sub_masks = ma->ptrs[(i * 64) + i2]->mask;

			for (int j = first_sub_mask;
			     j < (PG_MAC_TABLE_MASK_SIZE); ++j) {

				if (!sub_masks[j])
					continue;

				PG_FOREACH_BIT(sub_masks[j] & sub_unset,
					       j2) {
					it->key.bytes32[0] = (i * 64) + i2;
					it->key.part2 = (j * 64) + j2;
					return;
				}
				sub_unset = 0;
			}
			first_sub_mask = 0;
		}
		unset = 0;
	}
	it->is_end = 1;
}

static inline void pg_mac_table_iterator_init(struct pg_mac_table_iterator *it,
					      struct pg_mac_table *tbl)
{
	it->key.mac = 0;
	it->is_end = 0;
	it->table = tbl;

	/* If 0 is set we don't need to increment it iterator */
	if (pg_mac_table_ptr_get(tbl, it->key))
		return;
	pg_mac_table_iterator_next(it);
}

static inline int pg_mac_table_iterator_is_end(struct pg_mac_table_iterator *it)
{
	return it->is_end;
}

static inline union pg_mac pg_mac_table_iterator_get_key(
	struct pg_mac_table_iterator *it)
{
	return it->key;
}

static inline void *pg_mac_table_iterator_get(struct pg_mac_table_iterator *it)
{
	return pg_mac_table_ptr_get(it->table, it->key);
}

#define PG_MAC_TABLE_FOREACH(ma, key, val_type, val)			\
	struct pg_mac_table_iterator it;				\
	union pg_mac key;						\
	val_type val;							\
	for (pg_mac_table_iterator_init(&it, (ma));			\
	     !pg_mac_table_iterator_is_end(&it) &&			\
		     ((key = pg_mac_table_iterator_get_key(&it)).mac || 1) && \
		     ((val = pg_mac_table_iterator_get(&it)) || 1);	\
	     pg_mac_table_iterator_next(&it))


#endif
