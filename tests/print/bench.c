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

#include <glib.h>
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"

void test_benchmark_print(int argc, char **argv);

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int r;

	g_test_init(&argc, &argv, NULL);
	pg_start(argc, argv, &error);
	g_assert(!error);
	test_benchmark_print(argc, argv);
	r = g_test_run();
	pg_stop();
	return r;
}
