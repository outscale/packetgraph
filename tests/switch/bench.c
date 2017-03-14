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

#include "bench.h"
#include <glib.h>
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_assert(pg_start(argc, argv) >= 0);
	test_benchmark_switch(argc, argv);
	int r = g_test_run();

	pg_stop();
	return r;
}
