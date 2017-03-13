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
	FAIL = 2,
	VM = 4,
	VM_KEY = 8,
	HUGEPAGES = 16
};

void test_vhost(void);

extern char *glob_vm_path;
extern char *glob_vm_key_path;
extern char *glob_hugepages_path;
extern int glob_long_tests;

#endif
