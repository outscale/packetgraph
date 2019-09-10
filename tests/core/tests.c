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

#include <glib.h>

#include <rte_config.h>
#include <rte_common.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include "tests.h"

static void print_usage(void)
{
	printf("tests usage: [EAL options] -- [-help]\n");
	exit(0);
}

static uint64_t parse_args(int argc, char **argv)
{
	int i;
	int ret = 0;

	for (i = 0; i < argc; ++i) {
		if (!strcmp("-help", argv[i])) {
			ret |= PRINT_USAGE;
		} else {
			printf("tests: invalid option -- %s\n", argv[i]);
			return FAIL | PRINT_USAGE;
		}
	}
	return ret;
}

int main(int argc, char **argv)
{
	int ret;
	uint64_t test_flags;
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	ret = pg_start(argc, argv);
	g_assert(ret >= 0);
	g_assert(!pg_init_seccomp());

	/* accounting program name */
	ret += + 1;
	argc -= ret;
	argv += ret;
	test_flags = parse_args(argc, argv);
	if (test_flags & PRINT_USAGE)
		print_usage();
	g_assert(!(test_flags & FAIL));

	test_bitmask();
	test_error();
	test_mac();
	test_brick_core();
	test_brick_flow();
	test_pkts_count();
	test_brick_dot();
	test_hub();
	test_graph();

	return g_test_run();
}
