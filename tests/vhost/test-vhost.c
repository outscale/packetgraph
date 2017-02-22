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
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <rte_config.h>
#include <rte_hash_crc.h>

#include <packetgraph/common.h>
#include <packetgraph/vhost.h>
#include "collect.h"
#include "packets.h"
#include "utils/tests.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "brick-int.h"
#include "utils/mac.h"
#include "utils/qemu.h"
#include "tests.h"

#define NB_PKTS	1

static uint16_t ssh_port_id = 65000;

/* this test harness a Linux guest to check that packet are send and received
 * by the vhost brick. An ethernet bridge inside the guest will forward packets
 * between the two vhost-user virtio interfaces.
 */
static void test_vhost_flow_(int qemu_exit_signal)
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

	/* start vhost */
	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* instanciate brick */
	vhost_0 = pg_vhost_new("vhost-0", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
			       &error);
	g_assert(!error);
	g_assert(vhost_0);

	vhost_1 = pg_vhost_new("vhost-1", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
			       &error);
	g_assert(!error);
	g_assert(vhost_1);

	collect = pg_collect_new("collect", &error);
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

	qemu_pid = pg_util_spawn_qemu(socket_path_0, socket_path_1,
				      mac_addr_0, mac_addr_1,
				      glob_vm_path,
				      glob_vm_key_path,
				      glob_hugepages_path, &error);

	g_assert(!error);
	g_assert(qemu_pid);

	/* Prepare VM's bridge. */
#	define SSH(c) \
		g_assert(pg_util_ssh("localhost", ssh_port_id, glob_vm_key_path, c) == 0)
	SSH("brctl addbr br0");
	SSH("ifconfig br0 up");
	SSH("ifconfig ens4 up");
	SSH("ifconfig ens5 up");
	SSH("brctl addif br0 ens4");
	SSH("brctl addif br0 ens5");
	SSH("brctl setfd br0 0");
	SSH("brctl stp br0 off");
#	undef SSH
	ssh_port_id++;

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
	pg_brick_burst_to_east(vhost_0, 0, pkts,
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

	result_pkts = pg_brick_east_burst_get(collect, &pkts_mask, &error);
	g_assert(!error);
	g_assert(result_pkts);
	g_assert(pg_brick_rx_bytes(vhost_0) == 0);
	g_assert(pg_brick_tx_bytes(vhost_0) != 0);
	g_assert(pg_brick_rx_bytes(vhost_1) != 0);
	g_assert(pg_brick_tx_bytes(vhost_1) == 0);

	/* kill QEMU */
	pg_util_stop_qemu(qemu_pid, qemu_exit_signal);

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

static void test_vhost_flow(void)
{
	printf("------- test_vhost_flow SIGKILL ---------\n");
	test_vhost_flow_(SIGKILL);
	printf("------- test_vhost_flow SIGINT  ---------\n");
	test_vhost_flow_(SIGINT);
}

/* this test harness a Linux guest to check that packet are send and received
 * by the vhost brick. An ethernet bridge inside the guest will forward packets
 * between the two vhost-user virtio interfaces.
 */
static void test_vhost_multivm_(int qemu_exit_signal)
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

	/* start vhost */
	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* instanciate brick */
	vhost_00 = pg_vhost_new("vhost-00", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
				&error);
	g_assert(!error);
	g_assert(vhost_00);

	vhost_01 = pg_vhost_new("vhost-01", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
				&error);
	g_assert(!error);
	g_assert(vhost_01);

	vhost_10 = pg_vhost_new("vhost-10", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
				&error);
	g_assert(!error);
	g_assert(vhost_10);

	vhost_11 = pg_vhost_new("vhost-11", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
				&error);
	g_assert(!error);
	g_assert(vhost_11);

	collect0 = pg_collect_new("collect0", &error);
	g_assert(!error);
	g_assert(collect0);

	collect1 = pg_collect_new("collect1", &error);
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
	qemu_pid0 = pg_util_spawn_qemu(socket_path_00, socket_path_01,
				       mac_addr_00, mac_addr_01,
				       glob_vm_path, glob_vm_key_path,
				       glob_hugepages_path, &error);
	g_assert(qemu_pid0);
	g_assert(!error);

#	define SSH(c) \
		g_assert(pg_util_ssh("localhost", ssh_port_id, glob_vm_key_path, c) == 0)
	SSH("brctl addbr br0");
	SSH("ifconfig br0 up");
	SSH("ifconfig ens4 up");
	SSH("ifconfig ens5 up");
	SSH("brctl addif br0 ens4");
	SSH("brctl addif br0 ens5");
	SSH("brctl setfd br0 0");
	SSH("brctl stp br0 off");
#	undef SSH
	ssh_port_id++;

	qemu_pid1 = pg_util_spawn_qemu(socket_path_10, socket_path_11,
				       mac_addr_10, mac_addr_11,
				       glob_vm_path, glob_vm_key_path,
				       glob_hugepages_path, &error);
	g_assert(qemu_pid1);
	g_assert(!error);

	/* Prepare VM's bridge. */
#	define SSH(c) \
		g_assert(pg_util_ssh("localhost", ssh_port_id, glob_vm_key_path, c) == 0)
	SSH("brctl addbr br0");
	SSH("ifconfig br0 up");
	SSH("ifconfig ens4 up");
	SSH("ifconfig ens5 up");
	SSH("brctl addif br0 ens4");
	SSH("brctl addif br0 ens5");
	SSH("brctl setfd br0 0");
	SSH("brctl stp br0 off");
#	undef SSH
	ssh_port_id++;

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
	pg_brick_burst_to_east(vhost_00, 0, pkts,
			       pg_mask_firsts(NB_PKTS),
			       &error);
	g_assert(!error);
	pg_brick_burst_to_east(vhost_10, 0, pkts,
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

	/* kill QEMU */
	pg_util_stop_qemu(qemu_pid0, qemu_exit_signal);
	pg_util_stop_qemu(qemu_pid1, qemu_exit_signal);

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

#define VHOST_CNT 1000

static void test_vhost_fd(void)
{
	struct pg_brick *vhost[VHOST_CNT];
	struct pg_error *error = NULL;

	g_assert(pg_vhost_start("/tmp", &error) == 0);
	g_assert(!error);

	for (int j = 0; j < 10; j++) {
		for (int i = 0; i < VHOST_CNT; i++) {
			gchar *name = g_strdup_printf("vhost-%i", i);
			vhost[i] =
				pg_vhost_new(name,
					     PG_VHOST_USER_DEQUEUE_ZERO_COPY,
					     &error);
			g_free(name);
			g_assert(!error);
			g_assert(vhost[i]);
		}
		for (int i = 0; i < VHOST_CNT; i++) {
			pg_brick_destroy(vhost[i]);
			g_assert(!error);
		}
		sleep(1);
	}
	pg_vhost_stop();
}

#undef VHOST_CNT

static void test_vhost_fd_loop(void) {
	int i;

	for (i = 0; i < 3; i++)
		test_vhost_fd();
}

static void test_vhost_multivm(void)
{
	printf("------- test_vhost_multivm SIGKILL ---------\n");
	test_vhost_multivm_(SIGKILL);
	printf("------- test_vhost_multivm SIGINT ---------\n");
	test_vhost_multivm_(SIGINT);
}

struct qemu_test_params {
	const char *socket_path[2];
	struct pg_brick *vhost[2];
	struct pg_brick *collect;
	int killsig;
};

static void qemu_test(struct qemu_test_params *p)
{
	const char mac_0[18] = "52:54:00:12:34:11";
	const char mac_1[18] = "52:54:00:12:34:12";
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	struct pg_error *error = NULL;
	int qemu_pid;
	struct rte_mempool *mbuf_pool = pg_get_mempool();
	struct rte_mbuf **result_pkts;
	int exit_status;
	uint64_t pkts_mask;

	qemu_pid = pg_util_spawn_qemu(p->socket_path[0], p->socket_path[1],
				      mac_0, mac_1,
				      glob_vm_path,
				      glob_vm_key_path,
				      glob_hugepages_path, &error);
	g_assert(!error);
	g_assert(qemu_pid);

	/* Prepare VM's bridge. */
#	define SSH(c) \
	g_assert(!pg_util_ssh("localhost", ssh_port_id, \
			      glob_vm_key_path, c))
	SSH("brctl addbr br0");
	SSH("ifconfig br0 up");
	SSH("ifconfig ens4 up");
	SSH("ifconfig ens5 up");
	SSH("brctl addif br0 ens4");
	SSH("brctl addif br0 ens5");
	SSH("brctl setfd br0 0");
	SSH("brctl stp br0 off");
#	undef SSH
	ssh_port_id++;

	/* prepare packet to send */
	for (int i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		rte_pktmbuf_append(pkts[i], ETHER_MIN_LEN);
		/* set random dst/src mac address so the linux guest
		 * bridge will not filter them
		 */
		pg_set_mac_addrs(pkts[i],
				 "52:54:00:12:34:15",
				 "52:54:00:12:34:16");
		/* set size */
		pg_set_ether_type(pkts[i],
				  ETHER_MIN_LEN - ETHER_HDR_LEN - 4);
	}

	/* send packet to the guest via one interface */
	pg_brick_burst_to_east(p->vhost[0], 0, pkts,
			       pg_mask_firsts(NB_PKTS), &error);
	g_assert(!error);

	/* let the packet propagate and flow */
	for (int i = 0; i < 10; i++) {
		uint16_t count = 0;

		usleep(100000);
		pg_brick_poll(p->vhost[1], &count, &error);
		g_assert(!error);
		if (count)
			break;
	}

	result_pkts = pg_brick_east_burst_get(p->collect, &pkts_mask,
					      &error);
	g_assert(!error);
	g_assert(result_pkts);

	/* kill QEMU */
	kill(qemu_pid, p->killsig);
	waitpid(qemu_pid, &exit_status, 0);
	g_spawn_close_pid(qemu_pid);

	/* free result packets */
	pg_packets_free(result_pkts, pkts_mask);

	/* free sent packet */
	for (int i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);
}

static void test_vhost_reco(void)
{
	struct qemu_test_params params;
	struct pg_error *error = NULL;
	int ret;

	/* start vhost */
	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret);
	g_assert(!error);

	/* instanciate brick */
	params.vhost[0] = pg_vhost_new("vhost-0",
				       PG_VHOST_USER_DEQUEUE_ZERO_COPY,
				       &error);
	g_assert(!error);
	g_assert(params.vhost[0]);

	params.vhost[1] = pg_vhost_new("vhost-1",
				       PG_VHOST_USER_DEQUEUE_ZERO_COPY,
				       &error);
	g_assert(!error);
	g_assert(params.vhost[1]);

	params.collect = pg_collect_new("collect", &error);
	g_assert(!error);
	g_assert(params.collect);

	/* build the graph */
	pg_brick_link(params.collect, params.vhost[1], &error);
	g_assert(!error);

	params.socket_path[0] = pg_vhost_socket_path(params.vhost[0], &error);
	g_assert(!error);
	g_assert(params.socket_path[0]);
	g_assert(g_file_test(params.socket_path[0], G_FILE_TEST_EXISTS));

	params.socket_path[1] = pg_vhost_socket_path(params.vhost[1], &error);
	g_assert(!error);
	g_assert(params.socket_path[1]);
	g_assert(g_file_test(params.socket_path[1], G_FILE_TEST_EXISTS));

	/* Test all combinaisons of spawn/kill */
	uint8_t s;
	uint8_t a;
	uint8_t max = 3;
	uint8_t all_scenarios = pow(max, 2);

	for (s = 0; s < all_scenarios; s++) {
		printf("sequence test %u/%u ... \n", s, all_scenarios);
		for (a = 0; a < max; a++) {
			if (s & (1 << a)) {
				params.killsig = SIGINT;
				qemu_test(&params);
			} else {
				params.killsig = SIGKILL;
				qemu_test(&params);
			}
		}
	}

	/* break the graph */
	pg_brick_unlink(params.collect, &error);
	g_assert(!error);

	/* clean up */
	pg_brick_destroy(params.vhost[0]);
	g_assert(!error);
	pg_brick_destroy(params.vhost[1]);
	g_assert(!error);
	pg_brick_decref(params.collect, &error);
	g_assert(!error);

	/* stop vhost */
	pg_vhost_stop();
}

static void test_vhost_destroy(void)
{
	const char mac_addr_0[18] = "52:54:00:12:34:11";
	const char mac_addr_1[18] = "52:54:00:12:34:12";
	struct pg_brick *vhost_0, *vhost_1;
	const char *socket_path_0, *socket_path_1;
	struct pg_error *error = NULL;
	int ret, qemu_pid;

	/* start vhost */
	ret = pg_vhost_start("/tmp", &error);
	g_assert(ret == 0);
	g_assert(!error);

	/* instanciate brick */
	vhost_0 = pg_vhost_new("vhost-0", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
			       &error);
	g_assert(!error);
	g_assert(vhost_0);

	vhost_1 = pg_vhost_new("vhost-1", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
			       &error);
	g_assert(!error);
	g_assert(vhost_1);

	/* spawn QEMU */
	socket_path_0 = pg_vhost_socket_path(vhost_0, &error);
	g_assert(!error);
	g_assert(socket_path_0);
	socket_path_1 = pg_vhost_socket_path(vhost_1, &error);
	g_assert(!error);
	g_assert(socket_path_1);

	qemu_pid = pg_util_spawn_qemu(socket_path_0, socket_path_1,
				      mac_addr_0, mac_addr_1,
				      glob_vm_path,
				      glob_vm_key_path,
				      glob_hugepages_path, &error);

	g_assert(!error);
	g_assert(qemu_pid);

	/* try to destroy vhost brick before qemu is killed */
	pg_brick_destroy(vhost_0);
	g_assert(!error);
	pg_brick_destroy(vhost_1);
	g_assert(!error);

	/* kill QEMU */
	pg_util_stop_qemu(qemu_pid, SIGKILL);

	/* stop vhost */
	pg_vhost_stop();
}

/* qemu_duo_* create a graph to test ping/iperf/tcp/udp:
 * <qemu>--[vhost]--[vhost]--<qemu>
 *
 * 1. init graph with qemu_duo_new()
 * 2. ssh some commands using pg_util_ssh()
 * 3. clean graph with qemu_duo_destroy()
 */

struct qemu_duo_test_params {
	pthread_t graph_thread;
	int stop;
	struct pg_brick *vhost[2];
	uint16_t port[2];
	int qemu_pid[2];
	char *ip[2];
};

static void *qemu_duo_internal_thread(void *arg) {
	struct qemu_duo_test_params *p = (struct qemu_duo_test_params *)arg;
	struct pg_error *error = NULL;
	uint16_t count;

	p->stop = 0;
	while (!p->stop) {
		pg_brick_poll(p->vhost[0], &count, &error);
		pg_brick_poll(p->vhost[1], &count, &error);
	}
	return NULL;
}

static void qemu_duo_new(struct qemu_duo_test_params *p)
{
	struct pg_error *error = NULL;
	char *tmp;
	char *mac;
	const char *socket_path;

	g_assert(!pg_vhost_start("/tmp", &error));
	g_assert(!error);
	for (int i = 0; i < 2; i++) {
		tmp = g_strdup_printf("vhost-%i", i);
		p->vhost[i] = pg_vhost_new(tmp, 0, &error);
		g_free(tmp);
		g_assert(!error);
		g_assert(p->vhost[i]);
		socket_path = pg_vhost_socket_path(p->vhost[i], &error);
		g_assert(socket_path);
		g_assert(!error);

		mac = g_strdup_printf("52:54:00:12:34:%02i", i);
		p->qemu_pid[i] = pg_util_spawn_qemu(socket_path, NULL,
						    mac, NULL,
						    glob_vm_path,
						    glob_vm_key_path,
						    glob_hugepages_path,
						    &error);
		g_free(mac);
		g_assert(!error);
		g_assert(p->qemu_pid[i]);
		p->port[i] = ssh_port_id;
		ssh_port_id++;

		p->ip[i] = g_strdup_printf("42.0.0.%i", i + 1);
		g_assert(!pg_util_ssh("localhost", p->port[i], glob_vm_key_path,
				      "ifconfig ens4 up"));
		g_assert(!pg_util_ssh("localhost", p->port[i], glob_vm_key_path,
				      "ip addr add %s/24 dev ens4", p->ip[i]));
	}

	g_assert(!pg_brick_link(p->vhost[0], p->vhost[1], &error));
	g_assert(!error);
	g_assert(!pthread_create(&p->graph_thread, NULL,
				 &qemu_duo_internal_thread, p));
}

static void qemu_duo_destroy(struct qemu_duo_test_params *p)
{
	int exit_status;
	void *ret;
	p->stop = 1;
	pthread_join(p->graph_thread, &ret);
	for (int i = 0; i < 2; i++) {
		kill(p->qemu_pid[i], SIGKILL);
		waitpid(p->qemu_pid[i], &exit_status, 0);
		g_spawn_close_pid(p->qemu_pid[i]);
		g_free(p->ip[i]);
		pg_brick_destroy(p->vhost[i]);
	}
	pg_vhost_stop();
}

static void test_vhost_net_classics(void)
{
	struct qemu_duo_test_params p;

	qemu_duo_new(&p);
	for (int i = 0; i < 2; i++) {
		g_assert(!pg_util_ssh("localhost", p.port[i], glob_vm_key_path,
				      "ping -c 1 -W 3 %s", p.ip[(i + 1) % 2]));
		g_assert(!pg_util_ssh("localhost", p.port[(i + 1) % 2],
				      glob_vm_key_path, "iperf3 -s -D"));
		usleep(100000);
		g_assert(!pg_util_ssh("localhost", p.port[i], glob_vm_key_path,
				      "iperf3 -t 1 -c %s", p.ip[(i + 1) % 2]));
		usleep(100000);
		g_assert(!pg_util_ssh("localhost", p.port[i], glob_vm_key_path,
				      "iperf3 -t 1 -u -c %s",
				      p.ip[(i + 1) % 2]));
	}
	qemu_duo_destroy(&p);
}

void test_vhost(void)
{
	pg_test_add_func("/vhost/net_classics", test_vhost_net_classics);
	pg_test_add_func("/vhost/flow", test_vhost_flow);
	pg_test_add_func("/vhost/multivm", test_vhost_multivm);
	pg_test_add_func("/vhost/fd", test_vhost_fd_loop);
	if (glob_long_tests)
		pg_test_add_func("/vhost/reco", test_vhost_reco);
	pg_test_add_func("/vhost/destroy", test_vhost_destroy);
}
