/* Copyright 2019 Outscale SAS
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


#ifndef PG_BYTEORDER_H_
#define PG_BYTEORDER_H_

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

#endif
