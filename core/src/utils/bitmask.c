/* Copyright 2014-2015 Nodalink EURL
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

#include <packetgraph/utils/bitmask.h>

uint64_t pg_mask_firsts(uint8_t count)
{
	uint8_t i;
	uint64_t result = 0;

	for (i = 0; i < count; i++) {
		result <<= 1;
		result |= 0x1;
	}

	return result;
}

int pg_mask_count(uint64_t pkts_mask)
{
	return __builtin_popcountll(pkts_mask);
}
