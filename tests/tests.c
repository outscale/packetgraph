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

#include <glib.h>

#include <rte_config.h>
#include <rte_eal.h>
#include <rte_virtio_net.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "tests.h"


static void print_usage(void)
{
	printf("tests usage: [EAL options] -- [-quick-test] [-help]\n");
	exit(0);
}

static uint64_t parse_args(int argc, char **argv)
{
	int i;
	int ret = 0;

	for (i = 0; i < argc; ++i) {
		if (!strcmp("-quick-test", argv[i])) {
			ret |= QUICK_TEST;
		} else if (!strcmp("-help", argv[i])) {
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

	/* initialize DPDK */
	ret = rte_eal_init(argc, argv);
	g_assert(ret >= 0);
	/* accounting program name */
	ret += + 1;
	argc -= ret;
	argv += ret;
	test_flags = parse_args(argc, argv);
	if (test_flags & PRINT_USAGE)
		print_usage();
	g_assert(!(test_flags & FAIL));

	test_error();
	test_brick_core();
	test_brick_flow();
	test_vhost();
	test_switch(test_flags);
	test_diode();
	test_pkts_count();

	return g_test_run();
}
