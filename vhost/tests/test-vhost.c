/* Copyright 2015 Nodalink EURL
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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <rte_config.h>
#include <rte_hash_crc.h>

#include <packetgraph/common.h>
#include <packetgraph/vhost.h>
#include <packetgraph/collect.h>
#include <packetgraph/packets.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/brick.h>
#include <packetgraph/utils/mac.h>
#include <packetgraph/utils/qemu.h>
#include "tests.h"

#define NB_PKTS	1

/* this test harness a Linux guest to check that packet are send and received
 * by the vhost brick. An ethernet bridge inside the guest will forward packets
 * between the two vhost-user virtio interfaces.
 */
static void test_vhost_flow(void)
{
	const char mac_addr_0[18] = "52:54:00:12:34:11";
	const char mac_addr_1[18] = "52:54:00:12:34:12";
	struct rte_mempool *mbuf_pool = pg_get_mempool();
	struct pg_brick *vhost_0, *vhost_1, *collect;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	const char *socket_path_0, *socket_path_1;
	struct pg_error *error = NULL;
	struct rte_mbuf **result_pkts;
	int ret, qemu_pid, i;
	uint64_t pkts_mask;
	int exit_status;

	/* start vhost */
	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret);
	g_assert(!error);

	/* instanciate brick */
	vhost_0 = pg_vhost_new("vhost-0", 1, 1, EAST_SIDE, &error);
	g_assert(!error);
	g_assert(vhost_0);

	vhost_1 = pg_vhost_new("vhost-1", 1, 1, EAST_SIDE, &error);
	g_assert(!error);
	g_assert(vhost_1);

	collect = pg_collect_new("collect", 1, 1, &error);
	g_assert(!error);
	g_assert(collect);

	/* build the graph */
	pg_brick_link(collect, vhost_1, &error);
	g_assert(!error);

	/* spawn first QEMU */
	socket_path_0 = pg_vhost_socket_path(vhost_0, &error);
	g_assert(!error);
	g_assert(socket_path_0);
	socket_path_1 = pg_vhost_socket_path(vhost_1, &error);
	g_assert(!error);
	g_assert(socket_path_1);
	qemu_pid = pg_spawn_qemu(socket_path_0, socket_path_1,
				 mac_addr_0, mac_addr_1, glob_bzimage_path,
				 glob_cpio_path, glob_hugepages_path, &error);
	g_assert(!error);
	g_assert(qemu_pid);

	/* prepare packet to send */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		rte_pktmbuf_append(pkts[i], ETHER_MIN_LEN);
		/* set random dst/src mac address so the linux guest bridge
		 * will not filter them
		 */
		pg_set_mac_addrs(pkts[i],
			      "52:54:00:12:34:15", "52:54:00:12:34:16");
		/* set size */
		pg_set_ether_type(pkts[i], ETHER_MIN_LEN - ETHER_HDR_LEN - 4);
	}

	/* send packet to the guest via one interface */
	pg_brick_burst_to_east(vhost_0, 0, pkts, NB_PKTS,
			       pg_mask_firsts(NB_PKTS), &error);
	g_assert(!error);

	/* let the packet propagate and flow */
	for (i = 0; i < 10; i++) {
		uint16_t count = 0;

		usleep(100000);
		pg_brick_poll(vhost_1, &count, &error);
		g_assert(!error);
		if (count)
			break;
	}

	/* kill QEMU */
	kill(qemu_pid, SIGKILL);
	waitpid(qemu_pid, &exit_status, 0);
	g_spawn_close_pid(qemu_pid);

	result_pkts = pg_brick_east_burst_get(collect, &pkts_mask, &error);
	g_assert(!error);
	g_assert(result_pkts);

	/* free result packets */
	pg_packets_free(result_pkts, pkts_mask);

	/* free sent packet */
	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the graph */
	pg_brick_unlink(collect, &error);
	g_assert(!error);

	/* clean up */
	/* pg_brick_decref(vhost_0, &error); */
	pg_brick_destroy(vhost_0);
	g_assert(!error);
	pg_brick_destroy(vhost_1);
	/* pg_brick_decref(vhost_1, &error); */
	g_assert(!error);
	pg_brick_decref(collect, &error);
	g_assert(!error);

	/* stop vhost */
	pg_vhost_stop();
}

/* this test harness a Linux guest to check that packet are send and received
 * by the vhost brick. An ethernet bridge inside the guest will forward packets
 * between the two vhost-user virtio interfaces.
 */
static void test_vhost_multivm(void)
{
	const char mac_addr_00[18] = "52:54:00:12:34:11";
	const char mac_addr_01[18] = "52:54:00:12:34:12";
	const char mac_addr_10[18] = "52:54:00:12:34:21";
	const char mac_addr_11[18] = "52:54:00:12:34:22";
	struct rte_mempool *mbuf_pool = pg_get_mempool();
	struct pg_brick *vhost_00, *vhost_01, *collect0;
	struct pg_brick *vhost_10, *vhost_11, *collect1;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	const char *socket_path_00, *socket_path_01;
	const char *socket_path_10, *socket_path_11;
	struct pg_error *error = NULL;
	struct rte_mbuf **result_pkts;
	int ret, qemu_pid0, qemu_pid1, i;
	uint64_t pkts_mask;
	int exit_status;

	/* start vhost */
	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret);
	g_assert(!error);

	/* instanciate brick */
	vhost_00 = pg_vhost_new("vhost-00", 1, 1, EAST_SIDE, &error);
	g_assert(!error);
	g_assert(vhost_00);

	vhost_01 = pg_vhost_new("vhost-01", 1, 1, EAST_SIDE, &error);
	g_assert(!error);
	g_assert(vhost_01);

	vhost_10 = pg_vhost_new("vhost-10", 1, 1, EAST_SIDE, &error);
	g_assert(!error);
	g_assert(vhost_10);

	vhost_11 = pg_vhost_new("vhost-11", 1, 1, EAST_SIDE, &error);
	g_assert(!error);
	g_assert(vhost_11);

	collect0 = pg_collect_new("collect0", 1, 1, &error);
	g_assert(!error);
	g_assert(collect0);

	collect1 = pg_collect_new("collect1", 1, 1, &error);
	g_assert(!error);
	g_assert(collect1);

	/* build the graph */
	pg_brick_link(collect0, vhost_01, &error);
	g_assert(!error);

	pg_brick_link(collect1, vhost_11, &error);
	g_assert(!error);

	/* spawn QEMU */
	socket_path_00 = pg_vhost_socket_path(vhost_00, &error);
	g_assert(!error);
	g_assert(socket_path_00);
	socket_path_01 = pg_vhost_socket_path(vhost_01, &error);
	g_assert(!error);
	socket_path_10 = pg_vhost_socket_path(vhost_10, &error);
	g_assert(!error);
	g_assert(socket_path_10);
	socket_path_11 = pg_vhost_socket_path(vhost_11, &error);
	g_assert(!error);
	g_assert(socket_path_11);
	qemu_pid0 = pg_spawn_qemu(socket_path_00, socket_path_01,
				  mac_addr_00, mac_addr_01, glob_bzimage_path,
				  glob_cpio_path, glob_hugepages_path, &error);
	g_assert(qemu_pid0);
	g_assert(!error);
	qemu_pid1 = pg_spawn_qemu(socket_path_10, socket_path_11,
				  mac_addr_10, mac_addr_11, glob_bzimage_path,
				  glob_cpio_path, glob_hugepages_path, &error);
	g_assert(qemu_pid1);
	g_assert(!error);

	/* prepare packet to send */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		rte_pktmbuf_append(pkts[i], ETHER_MIN_LEN);
		/* set random dst/src mac address so the linux guest bridge
		 * will not filter them
		 */
		pg_set_mac_addrs(pkts[i],
			      "52:54:00:12:34:15", "52:54:00:12:34:16");
		/* set size */
		pg_set_ether_type(pkts[i], ETHER_MIN_LEN - ETHER_HDR_LEN - 4);
	}

	/* send packet to the guest via one interface */
	pg_brick_burst_to_east(vhost_00, 0, pkts, NB_PKTS,
			       pg_mask_firsts(NB_PKTS),
			       &error);
	g_assert(!error);
	pg_brick_burst_to_east(vhost_10, 0, pkts, NB_PKTS,
			       pg_mask_firsts(NB_PKTS),
			       &error);
	g_assert(!error);

	/* let the packet propagate and flow */
	for (i = 0; i < 10; i++) {
		uint16_t count0 = 0;
		uint16_t count1 = 0;

		usleep(100000);
		pg_brick_poll(vhost_01, &count0, &error);
		g_assert(!error);
		pg_brick_poll(vhost_11, &count1, &error);
		g_assert(!error);
		if (count0 && count1)
			break;
	}

	/* kill QEMU */
	kill(qemu_pid0, SIGKILL);
	waitpid(qemu_pid0, &exit_status, 0);
	kill(qemu_pid1, SIGKILL);
	waitpid(qemu_pid1, &exit_status, 0);
	g_spawn_close_pid(qemu_pid0);
	g_spawn_close_pid(qemu_pid1);

	result_pkts = pg_brick_east_burst_get(collect0, &pkts_mask, &error);
	g_assert(!error);
	g_assert(result_pkts);
	result_pkts = pg_brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(result_pkts);

	/* free result packets */
	pg_packets_free(result_pkts, pkts_mask);

	/* free sent packet */
	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the graph */
	pg_brick_unlink(collect0, &error);
	g_assert(!error);
	pg_brick_unlink(collect1, &error);
	g_assert(!error);

	/* clean up */
	pg_brick_destroy(vhost_00);
	g_assert(!error);
	pg_brick_destroy(vhost_01);
	g_assert(!error);
	pg_brick_destroy(vhost_10);
	g_assert(!error);
	pg_brick_destroy(vhost_11);
	g_assert(!error);
	pg_brick_decref(collect0, &error);
	g_assert(!error);
	pg_brick_decref(collect1, &error);
	g_assert(!error);

	/* stop vhost */
	pg_vhost_stop();
}

void test_vhost(void)
{
	g_test_add_func("/vhost/flow", test_vhost_flow);
	g_test_add_func("/vhost/multivm", test_vhost_multivm);
}
