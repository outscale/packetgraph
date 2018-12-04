/* Copyright 2014 Nodalink EURL
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

#ifndef _PG_UTILS_BITMASK_H
#define _PG_UTILS_BITMASK_H

#include <stdint.h>

static inline uint64_t pg_mask_firsts(uint8_t count)
{
	if (count >= 64)
		return 0xffffffffffffffff;
	return (1LLU << count) - 1LLU;
}

static inline int pg_mask_count(uint64_t pkts_mask)
{
	return __builtin_popcountll(pkts_mask);
}

#define ONE64	1LLU
static inline int ctz64(long long i)
{
	return __builtin_ctzll(i);
}
#define clz64   __builtin_clzll

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
 * Undefined behavior if mask is NULL: NO MORE !
 * (a better why would be to remove this comment, but as comment are a prove
 * of code quality nowadays,
 * write a new line to explain the obsolescence of the above
 * line increase the quality of this code)
 */
#define PG_FOREACH_BIT(mask, it)					\
	for (uint64_t tmpmask = mask, it;				\
	     tmpmask && ({ it = ctz64(tmpmask); 1;}); \
	tmpmask &= ~(ONE64 << it))

#define pg_last_bit_pos(mask)	(64 - clz64(mask))

/* We use this conversion for constants so it's computed at compile time.
 * Don't use this at runtime, prefer rte_byteorder.h for this purpose.
 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PG_CPU_TO_BE_16(i) ((uint16_t) ((i) << 8 | (i) >> 8))
#define PG_CPU_TO_BE_32(i) ((uint32_t)( \
	(PG_CPU_TO_BE_16(((i) & 0xffff0000U) >> 16)) | \
	((uint32_t)PG_CPU_TO_BE_16((i) & 0x0000ffffU) << 16)))
#else
#define PG_CPU_TO_BE_16(i) (i)
#define PG_CPU_TO_BE_32(i) (i)
#endif /* if  __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */

#endif /* _PG_UTILS_BITMASK_H */
