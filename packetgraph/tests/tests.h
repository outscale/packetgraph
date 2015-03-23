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

#ifndef _TESTS_H_
#define _TESTS_H_

enum test_flags {
	QUICK_TEST = 1,
	PRINT_USAGE = 2,
	FAIL = 4
};

void test_brick_core(void);
void test_brick_flow(void);
void test_error(void);
void test_switch(uint64_t test_flags);
void test_diode(void);
void test_vhost(void);
void test_pkts_count(void);

#endif
