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

#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>
#include <rte_config.h>
#include <rte_common.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include <packetgraph/vhost.h>
#include "packets.h"
#include "brick-int.h"
#include "utils/bench.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/qemu.h"

void test_benchmark_vhost(char *vm_image_path,
			  char *vm_ssh_key_path,
			  char *hugepages_path,
			  int argc,
			  char **argv);

void test_benchmark_vhost(char *vm_image_path,
			  char *vm_ssh_key_path,
			  char *hugepages_path,
			  int argc,
			  char **argv)
{
	struct pg_error *error = NULL;
	struct pg_brick *vhost_enter;
	struct pg_brick *vhost_exit;
	struct pg_bench bench;
	struct pg_bench_stats stats;
	const char *socket_path_enter;
	const char *socket_path_exit;
	struct ether_addr rand_mac1 = {{0x52, 0x54, 0x00, 0x00, 0x42, 0x01} };
	struct ether_addr rand_mac2 = {{0x52, 0x54, 0x00, 0x00, 0x42, 0x02} };
	const char mac1_str[18] = "52:54:00:12:34:11";
	const char mac2_str[18] = "52:54:00:12:34:12";
	int ret;
	int qemu_pid;

	g_assert(!pg_bench_init(&bench, "vhost", argc, argv, &error));
	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret == 0);
	g_assert(!error);

	vhost_enter = pg_vhost_new("vhost-enter",
				   PG_VHOST_USER_DEQUEUE_ZERO_COPY, &error);
	g_assert(!error);
	vhost_exit = pg_vhost_new("vhost-exit",
				  PG_VHOST_USER_DEQUEUE_ZERO_COPY, &error);
	g_assert(!error);

	socket_path_enter = pg_vhost_socket_path(vhost_enter);
	g_assert(socket_path_enter);
	socket_path_exit = pg_vhost_socket_path(vhost_exit);
	g_assert(socket_path_exit);

	qemu_pid = pg_util_spawn_qemu(socket_path_enter,
				      socket_path_exit,
				      mac1_str, mac2_str,
				      vm_image_path, vm_ssh_key_path,
				      hugepages_path, &error);
	g_assert(!error);
	g_assert(qemu_pid);

#	define SSH(c) \
		g_assert(pg_util_ssh("localhost", 65000, \
		vm_ssh_key_path, c) == 0)
	SSH("brctl addbr br0");
	SSH("ifconfig br0 up");
	SSH("ifconfig ens4 up");
	SSH("ifconfig ens5 up");
	SSH("brctl addif br0 ens4");
	SSH("brctl addif br0 ens5");
	SSH("brctl setfd br0 0");
	SSH("brctl stp br0 off");
#	undef SSH

	bench.input_brick = vhost_enter;
	bench.input_side = PG_WEST_SIDE;
	bench.output_brick = vhost_exit;
	bench.output_side = PG_WEST_SIDE;
	bench.output_poll = true;
	bench.max_burst_cnt = 20000000;
	bench.count_brick = NULL;
	bench.pkts_nb = 32;
	bench.pkts_mask = pg_mask_firsts(32);
	bench.pkts = pg_packets_create(bench.pkts_mask);
	bench.pkts = pg_packets_append_ether(
		bench.pkts,
		bench.pkts_mask,
		&rand_mac1, &rand_mac2,
		0xcafe);
	bench.pkts = pg_packets_append_blank(bench.pkts, bench.pkts_mask,
					     1500 - sizeof(struct ether_hdr));

	if (pg_bench_run(&bench, &stats, &error) < 0) {
		pg_error_print(error);
		abort();
	}

	pg_bench_print(&stats);
	pg_util_stop_qemu(qemu_pid, SIGKILL);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(vhost_enter);
	pg_brick_destroy(vhost_exit);
	pg_vhost_stop();
}

