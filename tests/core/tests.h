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

#ifndef _TESTS_H_
#define _TESTS_H_

enum test_flags {
	PRINT_USAGE = 1,
	FAIL = 2
};

void test_bitmask(void);
void test_brick_core(void);
void test_brick_dot(void);
void test_brick_flow(void);
void test_error(void);
void test_mac(void);
void test_pkts_count(void);
void test_benchmark_nop(void);
void test_hub(void);
void test_graph(void);

extern uint16_t  max_pkts;

#endif
