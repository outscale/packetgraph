/* Copyright 2015 Outscale SAS
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
#include <packetgraph/packets.h>
#include <packetgraph/brick.h>
#include <packetgraph/vhost.h>
#include <packetgraph/utils/bench.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/utils/qemu.h>

void test_benchmark_vhost(char *vm_image_path,
			  char *vm_ssh_key_path,
			  char *hugepages_path);

void test_benchmark_vhost(char *vm_image_path,
			  char *vm_ssh_key_path,
			  char *hugepages_path)
{
	struct pg_error *error = NULL;
	struct pg_brick *vhost_enter;
	struct pg_brick *vhost_exit;
	struct pg_bench bench;
	struct pg_bench_stats stats;
	const char *socket_path_enter;
	const char *socket_path_exit;
	struct ether_addr mac1 = {{0x52,0x54,0x00,0x12,0x34,0x11}};
	struct ether_addr mac2 = {{0x52,0x54,0x00,0x12,0x34,0x21}};
	const char mac1_str[18] = "52:54:00:12:34:11";
	const char mac2_str[18] = "52:54:00:12:34:12";
	uint32_t len;
	int ret;
	int qemu_pid;

	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret);
	g_assert(!error);

	vhost_enter = pg_vhost_new("vhost-enter", 1, 1, EAST_SIDE, &error);
	g_assert(!error);
	vhost_exit = pg_vhost_new("vhost-exit", 1, 1, EAST_SIDE, &error);
	g_assert(!error);

	socket_path_enter = pg_vhost_socket_path(vhost_enter, &error);
	g_assert(!error);
	g_assert(socket_path_enter);
	socket_path_exit = pg_vhost_socket_path(vhost_exit, &error);
	g_assert(!error);
	g_assert(socket_path_exit);

	qemu_pid = pg_util_spawn_qemu(socket_path_enter,
				      socket_path_exit,
				      mac1_str, mac2_str,
				      vm_image_path, vm_ssh_key_path,
				      hugepages_path, &error);
	g_assert(!error);
	g_assert(qemu_pid);

#	define SSH(c) \
		g_assert(!pg_util_ssh("localhost", 65000, vm_ssh_key_path, c))
	SSH("'yes | pacman -Sq bridge-utils'");
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
	bench.input_side = WEST_SIDE;
	bench.output_brick = vhost_exit;
	bench.output_side = WEST_SIDE;
	bench.output_poll = true;
	bench.max_burst_cnt = 1000000;
	bench.count_brick = NULL;
	bench.pkts_nb = 64;
	bench.pkts_mask = pg_mask_firsts(64);
	bench.pkts = pg_packets_create(bench.pkts_mask);
	bench.pkts = pg_packets_append_ether(
		bench.pkts,
		bench.pkts_mask,
		&mac1, &mac2,
		ETHER_TYPE_IPv4);
	len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) +
		sizeof(struct vxlan_hdr) + sizeof(struct ether_hdr) + 1400;
	pg_packets_append_ipv4(
		bench.pkts,
		bench.pkts_mask,
		0x000000EE, 0x000000CC, len, 17);
	bench.pkts = pg_packets_append_udp(
		bench.pkts,
		bench.pkts_mask,
		1000, 2000, 1400);
	bench.pkts = pg_packets_append_blank(bench.pkts, bench.pkts_mask, 1400);

	g_assert(pg_bench_run(&bench, &stats, &error));
	g_assert(pg_bench_print(&stats, NULL));

	pg_util_stop_qemu(qemu_pid);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(vhost_enter);
	pg_brick_destroy(vhost_exit);
	pg_vhost_stop();
}

