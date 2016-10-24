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

#ifndef _PG_UTILS_BITMASK_H
#define _PG_UTILS_BITMASK_H

#include <stdint.h>

uint64_t pg_mask_firsts(uint8_t count);

int pg_mask_count(uint64_t pkts_mask);

#if __SIZEOF_POINTER__ == 8
#define ONE64	1LU
#define ctz64	__builtin_ctzl
#define clz64	__builtin_clzl
#elif __SIZEOF_POINTER__ == 4
#define ONE64	1LLU
#define ctz64	__builtin_ctzll
#define clz64   __builtin_clzll
#endif

/**
 * Made as a macro for performance reason since a function would imply a 10%
 * performance hit and a function in a separate module would not be inlined.
 * Undefine behavior if mask is NULL
 */
#define pg_low_bit_iterate_full(mask, bit, index) do {	\
		index =  ctz64(mask);			\
		bit = ONE64 << index;			\
		mask &= ~bit;				\
	} while (0)

/**
 * Undefine behavior if mask is NULL
 */
#define pg_low_bit_iterate(mask, index) do {	\
		uint64_t bit;			\
		index =  ctz64(mask);		\
		bit = ONE64 << index;		\
		mask &= ~bit;			\
	} while (0)

/**
 * Undefine behavior if mask is NULL
 */
#define PG_FOREACH_BIT(mask, it)					\
	for (uint64_t tmpmask = mask, it;				\
	     ((it = ctz64(tmpmask)) || 1) &&				\
		     tmpmask;						\
	     tmpmask &= ~(ONE64 << it))

#define pg_last_bit_pos(mask)	(64 - clz64(mask))

#endif /* _PG_UTILS_BITMASK_H */
