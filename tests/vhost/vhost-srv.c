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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include "tests.h"

#include <packetgraph/vhost.h>
#include <packetgraph/common.h>

char *glob_vm_path;
char *glob_vm_key_path;
char *glob_hugepages_path;
int glob_long_tests;

static void print_usage(void)
{
	printf("tests usage: [EAL options] -- [-help] -vm /path/to/vm/image"
		" -vm-key /path/to/vm/ssh/key"
		"-hugepages /path/to/hugepages/mount\n");
	exit(0);
}

static uint64_t parse_args(int argc, char **argv)
{
	int i;
	int ret = 0;

	for (i = 0; i < argc; ++i) {
		if (!strcmp("-help", argv[i])) {
			ret |= PRINT_USAGE;
		} else if (!strcmp("-vm", argv[i]) && i + 1 < argc) {
			glob_vm_path = argv[i + 1];
			ret |= VM;
			i++;
		} else if (!strcmp("-vm-key", argv[i]) && i + 1 < argc) {
			glob_vm_key_path = argv[i + 1];
			ret |= VM_KEY;
			i++;
		} else if (!strcmp("-hugepages", argv[i]) && i + 1 < argc) {
			glob_hugepages_path = argv[i + 1];
			ret |= HUGEPAGES;
			i++;
		} else if (!strcmp("-long-tests", argv[i])) {
			glob_long_tests = 1;
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
	//struct pg_graph *graph;
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	ret = pg_start(argc, argv);
	g_assert(ret >= 0);
	/* accounting program name */
	ret += + 1;
	argc -= ret;
	argv += ret;
	test_flags = parse_args(argc, argv);
	if ((test_flags & (VM | VM_KEY | HUGEPAGES)) == 0)
		test_flags |= PRINT_USAGE;
	if (test_flags & PRINT_USAGE)
		print_usage();
	g_assert(!(test_flags & FAIL));

	struct pg_brick *vhost_00;//, *vhost_01;
	struct pg_error *error = NULL;
	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret == 0);
	g_assert(!error);
	vhost_00 = pg_vhost_new("vhost-X", 0, &error);
	const char* socket_path_0 = pg_vhost_socket_path(vhost_00);
	printf("%s\n-------------00\n", socket_path_0);
	g_assert(!error);
	//graph = pg_graph_new("example", vhost_00, &error);
	uint16_t prev_count = -1;

	while (40){
		uint16_t count = 0;

		pg_brick_poll(vhost_00, &count, &error);
		g_assert(!error);
		if(count != prev_count){
			prev_count = count;
			printf("polling... pkts received: %d\n", count);
		}
		if (count)
		break;
	}
	pg_error_print(error);
	pg_error_free(error);
	pg_stop();
	pg_brick_destroy(vhost_00);
	g_assert(!error);

	return -1;
}
