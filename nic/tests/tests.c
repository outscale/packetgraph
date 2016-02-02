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
#include <rte_common.h>
#include <rte_eal.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <packetgraph/packetgraph.h>
#include <packetgraph/nic.h>
#include "tests.h"

char *glob_pcap_in;

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
		if (!strcmp("-help", argv[i])) {
			ret |= PRINT_USAGE;
		} else if (!strcmp("--pcap-in", argv[i]) && i + 1 < argc) {
			glob_pcap_in = argv[i + 1];
			ret |= PCAP_IN;
			i++;
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
	struct pg_error **err = NULL;
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	ret = pg_start(argc, argv, err);
	g_assert(!err);
	pg_nic_start();
	/* accounting program name */
	ret += + 1;
	argc -= ret;
	argv += ret;
	test_flags = parse_args(argc, argv);

	if ((test_flags & PCAP_IN) == 0)
		test_flags |= PRINT_USAGE;
	if (test_flags & PRINT_USAGE)
		print_usage();
	g_assert(!(test_flags & FAIL));

	test_nic();
	return g_test_run();
}
