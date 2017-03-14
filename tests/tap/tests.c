/* Copyright 2016 Outscale SAS

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
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <glib.h>
#include <string.h>
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/mac.h"
#include "collect.h"
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"

#define CHECK_ERROR(error) do {                 \
	if (error) {                      \
		pg_error_print(error);   \
		g_assert(!error);	\
	}               \
} while (0)

static bool iface_exists(const char *ifname)
{
	struct ifaddrs *ifa_ori;
	struct ifaddrs *ifa;

	g_assert(ifname);
	g_assert(getifaddrs(&ifa_ori) == 0);
	for (ifa = ifa_ori; ifa != NULL; ifa = ifa->ifa_next) {
		if (g_strcmp0(ifname, ifa->ifa_name) == 0) {
			freeifaddrs(ifa_ori);
			return true;
		}
	}
	freeifaddrs(ifa_ori);
	return false;
}

static void test_tap_lifecycle(void)
{
	struct pg_brick *tap, *tap2;
	struct pg_error *error = NULL;
	const char *ifname;

	/* create tap */
	g_assert(!iface_exists("YAY"));
	tap = pg_tap_new("tap", "YAY", &error);
	g_assert(tap);
	g_assert(!error);
	ifname = pg_tap_ifname(tap);
	g_assert(strlen(ifname) > 0);
	g_assert(g_strcmp0("YAY", ifname) == 0);
	g_assert(iface_exists("YAY"));

	/* destroy tap */
	pg_brick_destroy(tap);
	g_assert(!iface_exists("YAY"));

	/* re-create tap */
	tap = pg_tap_new("tap", "YAY", &error);
	g_assert(tap);
	g_assert(!error);
	ifname = pg_tap_ifname(tap);
	g_assert(strlen(ifname) > 0);
	g_assert(g_strcmp0("YAY", ifname) == 0);
	g_assert(iface_exists("YAY"));

	/* try to create an existing tap */
	tap2 = pg_tap_new("tap2", "YAY", &error);
	g_assert(!tap2);
	g_assert(error);
	pg_error_free(error);
	error = NULL;

	/* create an other tap with different name */
	g_assert(!iface_exists("YOO"));
	tap2 = pg_tap_new("tap", "YOO", &error);
	g_assert(tap2);
	g_assert(!error);
	ifname = pg_tap_ifname(tap2);
	g_assert(strlen(ifname) > 0);
	g_assert(g_strcmp0("YOO", ifname) == 0);
	g_assert(iface_exists("YOO"));

	/* destroy tap */
	pg_brick_destroy(tap);
	g_assert(!iface_exists("YAY"));
	pg_brick_destroy(tap2);
	g_assert(!iface_exists("YOO"));

	/* re-create tap with no name*/
	int if_nb = 0;
	char *ifname_available;
	/* search next available tap name */
	while (42) {
		ifname_available = g_strdup_printf("tap%i", if_nb);
		if (!iface_exists(ifname_available))
			break;
		g_free(ifname_available);
		if_nb++;
	}
	tap = pg_tap_new("tap", NULL, &error);
	g_assert(tap);
	g_assert(!error);
	ifname = pg_tap_ifname(tap);
	g_assert(ifname);
	g_assert(strlen(ifname) > 0);
	g_assert(g_strcmp0(ifname, ifname_available) == 0);
	g_assert(iface_exists(ifname));
	pg_brick_destroy(tap);
	g_assert(!iface_exists(ifname));

	g_free(ifname_available);
}

static bool global_poll_run = true;
static void *pool(void *pv)
{
	struct pg_graph *g = (struct pg_graph *) pv;
	struct pg_error *error = NULL;

	while (global_poll_run) {
		pg_graph_poll(g, &error);
		if (error) {
			pg_error_print(error);
			g_assert(error);
		}
	}
	return NULL;
}

#define run_ok(command) g_assert(system("bash -c \"" command "\"") == 0)
#define run_ko(command) g_assert(system("bash -c \"" command "\"") != 0)
#define run(command) {int nop = system(command); (void) nop; }
static void test_tap_com(void)
{
	/**
	 * ping between two tap in two different network namespace
	 *
	 *               _________                _________
	 *               |       |                |       |
	 *   42.0.0.1/24 | pack0 +----------------+ pack1 | 42.0.0.2/24
	 *               |_______|                |_______|
	 *
	 *               | ns 0  |  packetgraph   | ns 1  |
	 */
	struct pg_brick *pack0, *pack1;
	struct pg_error *error = NULL;
	struct pg_graph *graph;

	pack0 = pg_tap_new("t0", "pack0", &error);
	g_assert(pack0);
	g_assert(!error);
	g_assert(iface_exists("pack0"));
	pack1 = pg_tap_new("t1", "pack1", &error);
	g_assert(pack1);
	g_assert(!error);
	g_assert(iface_exists("pack1"));

	g_assert(!pg_brick_chained_links(&error, pack0, pack1));
	g_assert(!error);
	graph = pg_graph_new("test", pack0, &error);
	g_assert(graph);
	g_assert(!error);

	/* clean previous netns */
	run("ip netns del ns0 &> /dev/null");
	run("ip netns del ns1 &> /dev/null");

	run_ok("ip netns add ns0");
	run_ok("ip link set pack0 up netns ns0");
	run_ok("ip netns exec ns0 ip link set dev lo up");
	run_ok("ip netns exec ns0 ip addr add 42.0.0.1/24 dev pack0");

	run_ok("ip netns add ns1");
	run_ok("ip link set pack1 up netns ns1");
	run_ok("ip netns exec ns1 ip link set dev lo up");
	run_ok("ip netns exec ns1 ip addr add 42.0.0.2/24 dev pack1");

	/* we don't pool packets ... check that we can't ping */
	run_ko("ip netns exec ns0 ping 42.0.0.2 -c 1 &> /dev/null");
	run_ko("ip netns exec ns1 ping 42.0.0.1 -c 1 &> /dev/null");

	g_thread_new("poll thread", &pool, graph);
	run_ok("ip netns exec ns0 ping 42.0.0.2 -c 3 &> /dev/null");
	run_ok("ip netns exec ns1 ping 42.0.0.1 -c 3 &> /dev/null");
	global_poll_run = false;

	pg_brick_unlink(pack0, &error);
	g_assert(!error);
	run_ko("ip netns exec ns0 ping 42.0.0.2 -c 1 &> /dev/null");
	run_ko("ip netns exec ns1 ping 42.0.0.1 -c 1 &> /dev/null");

	pg_graph_destroy(graph);
	g_assert(!iface_exists("pack0"));
	g_assert(!iface_exists("pack1"));
	run("ip netns del ns0");
	run("ip netns del ns1");
}

static void test_tap_mac(void)
{
	struct pg_brick *tap;
	struct pg_error *error = NULL;
	struct ether_addr mac;
	char tmp[18];

	tap = pg_tap_new("tap", "but-test-0", &error);
	g_assert(tap);
	g_assert(!error);
	run_ok("ip link set dev but-test-0 address 42:42:AB:AC:CA:FE");
	g_assert(!pg_tap_get_mac(tap, &mac));
	pg_printable_mac(&mac, tmp);
	g_assert(g_strcmp0(tmp, "42:42:AB:AC:CA:FE") == 0);
	pg_brick_destroy(tap);
}
#undef run_ok
#undef run_ko
#undef run

int main(int argc, char **argv)
{
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	g_assert(pg_start(argc, argv) >= 0);

	pg_test_add_func("/tap/lifecycle", test_tap_lifecycle);
	pg_test_add_func("/tap/com", test_tap_com);
	pg_test_add_func("/tap/mac", test_tap_mac);
	int r = g_test_run();

	pg_stop();
	return r;
}
