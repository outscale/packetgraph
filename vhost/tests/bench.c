/* Copyright 2015 Outscale SAS
 * Copyright 2014 Nodalink EURL
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

#include <stdlib.h>
#include <glib.h>
#include <packetgraph/packetgraph.h>

void test_benchmark_vhost(char *bzimage_path, char *cpio_path,
			  char *hugepages_path);
char *glob_bzimage_path;
char *glob_cpio_path;
char *glob_hugepages_path;

enum test_flags {
	PRINT_USAGE = 1,
	FAIL = 2,
	CPIO = 4,
	BZIMAGE = 8,
	HUGEPAGES = 16
};

static void print_usage(void)
{
	printf("benchmark usage: [EAL options] -- [-help] -bzimage /path/to/kernel -cpio /path/to/rootfs.cpio -hugepages /path/to/hugepages/mount\n");
	exit(0);
}

static uint64_t parse_args(int argc, char **argv)
{
	int i;
	int ret = 0;

	for (i = 0; i < argc; ++i) {
		if (!g_strcmp0("-help", argv[i])) {
			ret |= PRINT_USAGE;
		} else if (!g_strcmp0("-bzimage", argv[i]) && i + 1 < argc) {
			glob_bzimage_path = argv[i + 1];
			ret |= BZIMAGE;
			i++;
		} else if (!g_strcmp0("-cpio", argv[i]) && i + 1 < argc) {
			glob_cpio_path = argv[i + 1];
			ret |= CPIO;
			i++;
		} else if (!g_strcmp0("-hugepages", argv[i]) && i + 1 < argc) {
			glob_hugepages_path = argv[i + 1];
			ret |= HUGEPAGES;
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

	/* initialize packetgraph */
	ret = pg_start(argc, argv, err);
	g_assert(!err);
	/* accounting program name */
	ret += + 1;
	argc -= ret;
	argv += ret;
	test_flags = parse_args(argc, argv);
	if ((test_flags & (BZIMAGE | CPIO | HUGEPAGES)) == 0)
		test_flags |= PRINT_USAGE;
	if (test_flags & PRINT_USAGE)
		print_usage();
	g_assert(!(test_flags & FAIL));

	test_benchmark_vhost(glob_bzimage_path,
			     glob_cpio_path,
			     glob_hugepages_path);

	return g_test_run();
}
