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

#ifndef _PG_CORE_UTILS_BITMASK_H
#define _PG_CORE_UTILS_BITMASK_H

#include <stdint.h>

uint64_t pg_mask_firsts(uint8_t count);

int pg_mask_count(uint64_t pkts_mask);

/**
 * Made as a macro for performance reason since a function would imply a 10%
 * performance hit and a function in a separate module would not be inlined.
 */
#define pg_low_bit_iterate_full(mask, bit, index) do {	\
	index =  __builtin_ctzll(mask);			\
	bit = 1LLU << index;				\
	mask &= ~bit;					\
	} while (0)

#define pg_low_bit_iterate(mask, index) do {	\
	uint64_t bit;				\
	index =  __builtin_ctzll(mask);		\
	bit = 1LLU << index;			\
	mask &= ~bit;				\
	} while (0)

#define PG_FOREACH_BIT(mask, it)					\
	for (uint64_t tmpmask = mask, it;				\
	     ((it = __builtin_ctzl(tmpmask)) || 1) &&			\
		     tmpmask;						\
	     tmpmask &= ~(1LLU << it))

#endif /* _PG_CORE_UTILS_BITMASK_H */
