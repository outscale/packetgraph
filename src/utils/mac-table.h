/* Copyright 2016 Outscale SAS
 *
 * This file is part of Packetgraph.
 *
 * Packetgraph is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * Packetgraph is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Packetgraph.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <rte_memcpy.h>
#include <setjmp.h>
#include <assert.h>
#include "utils/mac.h"
#include "utils/bitmask.h"
#include "utils/malloc.h"

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
 * A mac array containing pointers or elements
 * the idea of this mac table, is that a mac is an unique identifier,
 * as sure, doesn't need hashing we could just
 * allocate an array for each possible mac
 * Problem is that doing so require ~280 TByte
 * So I've cut the mac in 2 part, example 01.02.03.04.05.06
 * will now have "01.02.03" that will serve as index of the mac table
 * and "04.05.06" will serve as the index of the sub mac table
 * if order to take advantage of Virtual Memory, we use bitmask, so we
 * don't have to allocate 512 MB of physical ram for each unlucky mac.
 */
struct pg_mac_table {
	uint64_t mask[PG_MAC_TABLE_MASK_SIZE];
	union {
		struct pg_mac_table_ptr **ptrs;
		struct pg_mac_table_elem **elems;
	};
	jmp_buf *exeption_env;
};

/**
 * iterate over a mac table, can't be done using a simple int
 * as mac table aren't simple array.
 * a mac table is composed by 1 container of sub container of elements/pointers
 * i is the index of the main container j, of the current sub container
 * m0 is current mask of main container and m1 of current sub container
 * mi/mj are index of current mask
 */
struct pg_mac_table_iterator {
	uint32_t i;
	uint32_t j;
	uint64_t m0;
	uint64_t m1;
	uint32_t mj;
	uint32_t mi;
	union {
		void *v_ptr;
		int8_t *c_ptr;
	};
	struct pg_mac_table *tbl;
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


static inline int pg_mac_table_init(struct pg_mac_table *ma,
				    jmp_buf *exeption_env)
{
	ma->ptrs = pg_malloc((0xffffff + 1) *
			     sizeof(struct pg_mac_table_ptr *));
	if (!ma->ptrs)
		return -1;
	memset(ma->mask, 0, (PG_MAC_TABLE_MASK_SIZE) * sizeof(uint64_t));
	ma->exeption_env = exeption_env;
	return 0;
}

static inline size_t pg_mac_table_compute_length(struct pg_mac_table *ma)
{
	size_t r = 0;

	for (uint64_t i = 0, m = ma->mask[i];
	     i < PG_MAC_TABLE_MASK_SIZE; m = ma->mask[++i]) {
		PG_FOREACH_BIT(m, it) {
			struct pg_mac_table_elem *imt = ma->elems[i * 64 + it];

			for (uint64_t j = 0;
			     j < PG_MAC_TABLE_MASK_SIZE; ++j) {
				uint64_t im = imt->mask[j];

				r += pg_mask_count(im);
			}
		}
	}

	return r;
}

static inline void pg_mac_table_clear(struct pg_mac_table *ma)
{
	if (unlikely(!ma))
		return;
	for (int i = 0; i < (PG_MAC_TABLE_MASK_SIZE); ++i) {
		if (likely(!ma->mask[i]))
			continue;
		PG_FOREACH_BIT(ma->mask[i], it) {
			struct pg_mac_table_ptr *cur = ma->ptrs[i * 64 + it];

			assert(cur);
			free(cur->entries);
			/* Uselsess but make -fanalyzer shut up */
			cur->entries = NULL;
			free(cur);
			ma->ptrs[i * 64 + it] = NULL;
		}
		ma->mask[i] = 0;
	}
}

static inline void pg_mac_table_free(struct pg_mac_table *ma)
{
	if (unlikely(!ma))
		return;
	for (int i = 0; i < (PG_MAC_TABLE_MASK_SIZE); ++i) {
		if (!ma->mask[i])
			continue;
		PG_FOREACH_BIT(ma->mask[i], it) {
			struct pg_mac_table_ptr *cur = ma->ptrs[i * 64 + it];

			assert(cur);
			free(cur->entries);
			cur->entries = NULL;
			free(cur);
			ma->ptrs[i * 64 + it] = NULL;
		}
	}
	free(ma->ptrs);
}

#define pg_mac_table_part2(mac) (mac.part2 & 0x00ffffff)
#define pg_mac_table_part1(mac) (mac.bytes32[0] & 0x00ffffff)

static inline void pg_mac_table_alloc_fail_exeption(struct pg_mac_table *ma)
{
	if (!ma->exeption_env) {
		fprintf(stderr,
			"pg_mac_table: malloc fail and exeption_env is unset");
		abort();
	}
	longjmp(*ma->exeption_env, 1);
}

static inline void pg_mac_table_elem_set(struct pg_mac_table *ma,
					 union pg_mac mac, void *entry,
					 size_t elem_size)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	/* Part 1 is unset */
	if (unlikely(!pg_mac_table_is_set(*ma, part1))) {
		ma->elems[part1] = pg_malloc(sizeof(struct pg_mac_table_elem));
		if (unlikely(!ma->elems[part1]))
			pg_mac_table_alloc_fail_exeption(ma);
		pg_mac_table_mask_set(*ma, part1);
		memset(ma->ptrs[part1]->mask, 0,
		       (PG_MAC_TABLE_MASK_SIZE) * sizeof(uint64_t));
		ma->elems[part1]->entries = pg_malloc((0xffffff + 1) *
						      elem_size);
		if (unlikely(!ma->elems[part1]->entries))
			pg_mac_table_alloc_fail_exeption(ma);
	}

	pg_mac_table_mask_set(*ma->elems[part1], part2);
	rte_memcpy(ma->elems[part1]->entries + (part2 * elem_size),
		   entry, elem_size);
}

static inline void pg_mac_table_ptr_set(struct pg_mac_table *ma,
					union pg_mac mac, void *entry)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	/* Part 1 is unset */
	if (unlikely(!pg_mac_table_is_set(*ma, part1))) {
		ma->ptrs[part1] = pg_malloc(sizeof(struct pg_mac_table_ptr));
		if (unlikely(!ma->ptrs[part1]))
			pg_mac_table_alloc_fail_exeption(ma);
		pg_mac_table_mask_set(*ma, part1);
		memset(ma->ptrs[part1]->mask, 0,
		       (PG_MAC_TABLE_MASK_SIZE) * sizeof(uint64_t));
		ma->ptrs[part1]->entries =
			pg_malloc((0xffffff + 1) * sizeof(void *));
		if (unlikely(!ma->ptrs[part1]->entries))
			pg_mac_table_alloc_fail_exeption(ma);
	}

	pg_mac_table_mask_set(*ma->ptrs[part1], part2);
	ma->ptrs[part1]->entries[part2] = entry;
}

static inline int pg_mac_table_ptr_unset(struct pg_mac_table *ma,
					  union pg_mac mac)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	if (!pg_mac_table_is_set(*ma, part1) ||
	    !pg_mac_table_is_set(*ma->ptrs[part1], part2))
		return -1;
	pg_mac_table_mask_unset(*ma->ptrs[part1], part2);
	return 0;
}

static inline int pg_mac_table_elem_unset(struct pg_mac_table *ma,
					   union pg_mac mac)
{
	uint32_t part1 = pg_mac_table_part1(mac);
	uint32_t part2 = pg_mac_table_part2(mac);

	if (!pg_mac_table_is_set(*ma, part1) ||
	    !pg_mac_table_is_set(*ma->elems[part1], part2))
		return -1;
	pg_mac_table_mask_unset(*ma->elems[part1], part2);
	return 0;
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

#define MAC_TABLE_IT_NEXT pg_mac_table_iterator_e_next
#define MAC_TABLE_TYPE elem
#define MAC_TABLE_SETTER(inner_t, useless) (it->c_ptr = (inner_t)->entries);

#include "mac-table-it-next.h"

#define MAC_TABLE_SETTER(inner_t, idx) (it->v_ptr = (inner_t)->entries[idx]);
#define MAC_TABLE_TYPE ptr
#define MAC_TABLE_IT_NEXT pg_mac_table_iterator_next

#include "mac-table-it-next.h"


static inline int pg_mac_table_iterator_is_end(struct pg_mac_table_iterator *it)
{
	return it->i == UINT32_MAX;
}

static inline union pg_mac
pg_mac_table_iterator_mk_key(struct pg_mac_table_iterator *it,
			     union pg_mac *k)
{
	k->bytes32[0] = (it->i * 64) + it->mi;
	k->part2 = (it->j * 64) + it->mj;
	return *k;
}

static inline void *pg_mac_table_iterator_get(struct pg_mac_table_iterator *it)
{
	return it->v_ptr;
}

#define pg_mac_table_iterator_e_get(it, type)				\
	((type *) ( (it).c_ptr + (((it.j * 64) + it.mj) * sizeof(type)) ) )

#define PG_MAC_TABLE_FOREACH_ELEM(ma, key, val_type, val)		\
	union pg_mac key = {.mac = 0};					\
	val_type *val = NULL;						\
	for (struct pg_mac_table_iterator it =				\
		     pg_mac_table_iterator_elem_create			\
		     ((ma), pg_mac_table_iterator_e_next);		\
	     !pg_mac_table_iterator_is_end(&it) &&			\
		     ({							\
			     key = pg_mac_table_iterator_mk_key(&it, &key) ; \
			     val = pg_mac_table_iterator_e_get(it, val_type); \
			     1;						\
		     });						\
	     pg_mac_table_iterator_e_next(&it))

#define PG_MAC_TABLE_FOREACH_PTR(ma, key, val_type, val)		\
	union pg_mac key = {.mac = 0};					\
	val_type *val = NULL;						\
	for (struct pg_mac_table_iterator it =				\
		     pg_mac_table_iterator_ptr_create			\
		     ((ma), pg_mac_table_iterator_next);		\
	     !pg_mac_table_iterator_is_end(&it) &&			\
		     ({key = pg_mac_table_iterator_mk_key(&it, &key); 1;}) && \
		     ((val = pg_mac_table_iterator_get(&it)) || 1);	\
	     pg_mac_table_iterator_next(&it))


#endif
