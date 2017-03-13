/* Copyright 2015 Outscale SAS
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

#ifndef _PG_DPDK_SYMBOLS_H
#define _PG_DPDK_SYMBOLS_H

#define PG_DPDK_INIT(function) \
	extern void function(void); \
	void (*pg_glob_ ##function)(void) __attribute__((unused)) = &function;

#include "dpdk_symbols_autogen.h"

#endif /* _PG_DPDK_SYMBOLS_H */
