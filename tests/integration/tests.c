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

#include <rte_config.h>
#include <rte_ring.h>
#include <rte_eth_ring.h>
#include <rte_ip.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_udp.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>

#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include <packetgraph/common.h>
#include <packetgraph/nic.h>
#include <packetgraph/vtep.h>
#include <packetgraph/vhost.h>
#include <packetgraph/print.h>
#include <packetgraph/firewall.h>
#include <packetgraph/antispoof.h>
#include <packetgraph/errors.h>
#include <packetgraph/graph.h>
#include "packets.h"
#include "brick-int.h"
#include "collect.h"
#include "utils/mempool.h"
#include "utils/config.h"
#include "utils/mac.h"
#include "utils/qemu.h"

uint16_t max_pkts = 64;

enum test_flags {
	PRINT_USAGE = 1,
	FAIL = 2,
	VM = 4,
	VM_KEY = 8,
	HUGEPAGES = 16
};

struct branch {
	struct pg_brick *vhost;
	struct pg_brick *collect;
	struct pg_brick *vhost_reader;
	struct pg_brick *print;
	struct pg_brick *firewall;
	struct pg_brick *antispoof;
	struct ether_addr mac;
	uint32_t id;
	GList *collector;
};

#define VNI_1	1
#define VNI_2	2

static char *glob_vm_path;
static char *glob_vm_key_path;
static char *glob_hugepages_path;

#define ASSERT(check) do {						\
		if (!(check)) {						\
			dprintf(2, "line=%d 'function=%s': assertion fail\n", \
				__LINE__, __func__);		\
			goto exit;					\
		}							\
	} while (0)

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)


#define CHECK_ERROR_ASSERT(error) do {		\
		if (error) {			\
			pg_error_print(error);	\
			ASSERT(0);		\
		}				\
	} while (0)


static void print_usage(void)
{
	printf("tests usage: [EAL options] -- [-help] -vm /path/to/vm/image"
		" -vm-key /path/to/vm/key -hugepages"
		"/path/to/hugepages/mount\n");
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
		} else {
			printf("tests: invalid option -- %s\n", argv[i]);
			return FAIL | PRINT_USAGE;
		}
	}
	return ret;
}

static int ring_port(void)
{
	static struct rte_ring *r1;
	static int port1;

	if (r1)
		return port1;
	r1 = rte_ring_create("R1", 256, 0, RING_F_SP_ENQ|RING_F_SC_DEQ);
	port1 = rte_eth_from_rings("r1-port", &r1, 1, &r1, 1, 0);
	return port1;
}

static inline const char *sock_path_graph(struct branch *branch)
{
	return pg_vhost_socket_path(branch->vhost);
}

static inline const char *sock_read_path_graph(struct branch *branch)
{
	return pg_vhost_socket_path(branch->vhost_reader);
}

static inline int start_qemu_graph(struct branch *branch,
				  const char *mac_reader,
				  struct pg_error **errp)
{
	static uint32_t ssh_port_id = 65000;
	char tmp_mac[20];

	g_assert(pg_printable_mac(&branch->mac, tmp_mac));
	int qemu_pid = pg_util_spawn_qemu(sock_path_graph(branch),
					  sock_read_path_graph(branch),
					  tmp_mac, mac_reader,
					  0,0,
					  glob_vm_path,
					  glob_vm_key_path,
					  glob_hugepages_path, errp);


#	define SSH(c) do {						\
		if (pg_util_ssh("localhost", ssh_port_id,		\
				glob_vm_key_path, c) < 0) {		\
			*errp =	pg_error_new("ssh failed: %s\n",c);	\
			return qemu_pid;				\
		}							\
	} while (0)
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
	return qemu_pid;
}


static int add_graph_branch(struct branch *branch, uint32_t id,
			    struct ether_addr mac,
			    int antispoof, int print)
{
	struct pg_error *error = NULL;
	GString *tmp = g_string_new(NULL);

	branch->collector = NULL;
	branch->id = id;
	branch->mac = mac;
	g_string_printf(tmp, "fw-%d", id);
	branch->firewall = pg_firewall_new(tmp->str, PG_NO_CONN_WORKER, &error);
	CHECK_ERROR(error);

	if (antispoof) {
		g_string_printf(tmp, "antispoof-%d", id);
		branch->antispoof = pg_antispoof_new(tmp->str, PG_EAST_SIDE,
						     &mac, &error);
		CHECK_ERROR(error);
	}

	g_string_printf(tmp, "vhost-%d", id);
	branch->vhost = pg_vhost_new(tmp->str, 0, &error);
	CHECK_ERROR(error);

	g_string_printf(tmp, "vhost-reader-%d", id);
	branch->vhost_reader = pg_vhost_new(tmp->str, 0, &error);
	CHECK_ERROR(error);

	g_string_printf(tmp, "collect-reader-%d", id);
	branch->collect = pg_collect_new(tmp->str, &error);
	CHECK_ERROR(error);

	if (print) {
		g_string_printf(tmp, "print-%d", id);
		branch->print = pg_print_new(tmp->str, NULL,
					     PG_PRINT_FLAG_MAX,
					     NULL, &error);
		CHECK_ERROR(error);
	}

	if (print && antispoof) {
		pg_brick_chained_links(&error, branch->firewall,
				       branch->antispoof, branch->print,
				       branch->vhost);
	} else if (print) {
		pg_brick_chained_links(&error, branch->firewall,
				       branch->print, branch->vhost);
	} else if (antispoof) {
		pg_brick_chained_links(&error, branch->firewall,
				       branch->antispoof,
				       branch->vhost);
	} else {
		pg_brick_chained_links(&error, branch->firewall,
				       branch->vhost);
	}
	CHECK_ERROR(error);
	pg_brick_chained_links(&error, branch->vhost_reader,
			       branch->collect);
	CHECK_ERROR(error);
	g_string_free(tmp, 1);
	return 1;
}

static int link_graph_branch(struct branch *branch, struct pg_brick *west_brick)
{
	struct pg_error *error = NULL;

	pg_brick_chained_links(&error, west_brick, branch->firewall);
	CHECK_ERROR(error);
	return 1;
}

static void test_graph_type1(void)
{
	struct pg_brick *nic, *vtep, *print;
	struct pg_error *error = NULL;
	struct ether_addr mac_vtep = {{0xb0, 0xb1, 0xb2,
				       0xb3, 0xb4, 0xb5} };
	struct ether_addr mac1 = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x11} };
	struct ether_addr mac2 = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x21} };
	const char mac_reader_1[18] = "52:54:00:12:34:12";
	const char mac_reader_2[18] = "52:54:00:12:34:22";
	struct pg_graph *graph = NULL, *graph1 = NULL, *graph2 = NULL;
	struct pg_graph *graph3 = NULL;
	struct branch branch1, branch2;
	int qemu1_pid = 0;
	int qemu2_pid = 0;
	struct rte_mbuf **pkts = NULL;
	struct rte_mbuf **result_pkts;
	uint64_t pkts_mask = 0, count = 0;
	int ret = -1;
	uint32_t len;

	pg_vhost_start("/tmp", &error);
	CHECK_ERROR(error);

	nic = pg_nic_new_by_id("nic", ring_port(), &error);
	if (error)
		pg_error_print(error);
	CHECK_ERROR(error);
	vtep = pg_vtep_new("vt", 50, PG_WEST_SIDE,
			   0x000000EE, mac_vtep, PG_VTEP_DST_PORT,
			   PG_VTEP_ALL_OPTI, &error);
	CHECK_ERROR(error);

	print = pg_print_new("print", NULL, PG_PRINT_FLAG_MAX,
			     NULL, &error);
	CHECK_ERROR(error);

	g_assert(add_graph_branch(&branch1, 1, mac1, 0, 0));
	g_assert(add_graph_branch(&branch2, 2, mac2, 0, 0));

	/*
	 * Our main graph:
	 *	                FIREWALL1 -- ANTISPOOF1 -- VHOST1
	 *		      /
	 * NIC -- PRINT-- VTEP
	 *	              \ FIREWALL2 -- ANTISPOOF2 -- VHOST2
	 *
	 *
	 * Todo this graph is the goal for this test, but we
	 * don't use ANTISPOOF in the graph yet.
	 *
	 */
	pg_brick_chained_links(&error, nic, print, vtep);
	CHECK_ERROR(error);

	g_assert(link_graph_branch(&branch1, vtep));
	g_assert(link_graph_branch(&branch2, vtep));

	graph = pg_graph_new("graph_1", nic, &error);
	CHECK_ERROR(error);
	graph1 = pg_graph_new("graph_2", branch1.vhost_reader, &error);
	CHECK_ERROR(error);
	graph3 = pg_graph_new("graph_3", branch2.vhost_reader, &error);
	CHECK_ERROR(error);

	g_assert(g_file_test(sock_path_graph(&branch1),
			     G_FILE_TEST_EXISTS));
	g_assert(g_file_test(sock_read_path_graph(&branch1),
			     G_FILE_TEST_EXISTS));
	g_assert(g_file_test(sock_path_graph(&branch2),
			     G_FILE_TEST_EXISTS));
	g_assert(g_file_test(sock_read_path_graph(&branch2),
			     G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_vm_path, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_vm_key_path, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_hugepages_path, G_FILE_TEST_EXISTS));
	/* spawm time ! */
	qemu1_pid = start_qemu_graph(&branch1, mac_reader_1, &error);
	CHECK_ERROR_ASSERT(error);
	g_assert(qemu1_pid);
	qemu2_pid = start_qemu_graph(&branch2, mac_reader_2, &error);
	CHECK_ERROR_ASSERT(error);
	g_assert(qemu2_pid);

	/* Now we need to kill qemu before exit in case of error */

	/* Add VNI's */
	pg_vtep_add_vni(vtep, branch1.firewall, VNI_1,
			inet_addr("225.0.0.1"), &error);
	CHECK_ERROR_ASSERT(error);
	pg_vtep_add_vni(vtep, branch2.firewall, VNI_2,
			inet_addr("225.0.0.2"), &error);
	CHECK_ERROR_ASSERT(error);

	/* Add firewall rule */
	ASSERT(pg_firewall_rule_add(branch1.firewall, "icmp",
				     PG_MAX_SIDE, 1, &error) == 0);
	ASSERT(pg_firewall_rule_add(branch2.firewall, "icmp",
				     PG_MAX_SIDE, 1, &error) == 0);

	len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) +
		sizeof(struct vxlan_hdr) + sizeof(struct ether_hdr) +
		sizeof("hello :)");
	/* forge a packte with VNI1 IP and a valide dest for the firewall */
	pkts = pg_packets_append_ether(pg_packets_create(1),
				       1, &mac_vtep, &mac_vtep,
				       ETHER_TYPE_IPv4);
	/* udp protocol number is 17 */
	pg_packets_append_udp(pg_packets_append_ipv4(pkts, 1,
						     0x000000EE,
						     0x000000CC, len, 17),
			      1, 50000, PG_VTEP_DST_PORT,
			      len - sizeof(struct ipv4_hdr));
	pg_packets_append_vxlan(pkts, 1, VNI_1);
	pg_packets_append_ether(pkts, 1, &mac2, &mac_vtep, 4);
	pg_packets_append_str(pkts, 1, "hello :)");

	/* Write the packet into the ring */
	rte_eth_tx_burst(ring_port(), 0, pkts, 1);
	g_assert(pg_graph_poll(graph, &error) == 0);
	CHECK_ERROR_ASSERT(error);

	/* Check all step of the transmition :) */
	ASSERT(pg_brick_pkts_count_get(nic, PG_WEST_SIDE));
	ASSERT(pg_brick_pkts_count_get(vtep, PG_EAST_SIDE));
	ASSERT(pg_brick_pkts_count_get(branch1.firewall, PG_EAST_SIDE));
	ASSERT(pg_brick_pkts_count_get(branch1.vhost, PG_EAST_SIDE));
	/* check the collect1 */
	for (int i = 0; i < 10; i++) {
		usleep(100000);
		g_assert(pg_graph_poll(graph1, &error) == 0);
		CHECK_ERROR_ASSERT(error);
		count = pg_brick_pkts_count_get(branch1.collect, PG_EAST_SIDE);
		if (count)
			break;
	}
	ASSERT(count);
	/* same with VNI 2 */
	result_pkts = pg_brick_west_burst_get(branch1.collect, &pkts_mask,
					      &error);
	ASSERT(result_pkts);
	/* split graph */
	graph2 = pg_graph_split(graph, "graph-2", "print", "vt",
						"q_west", "q_east", &error);
	CHECK_ERROR(error);
	/* merge graph after splitted */
	g_assert(pg_graph_merge(graph, graph2, &error) == 0);
	CHECK_ERROR(error);
	g_assert(pg_graph_poll(graph, &error) == 0);
	CHECK_ERROR_ASSERT(error);

	/* Write in vhost2_reader */
	/* Check all step of the transmition (again in the oposit way) */
	/* Check the ring has recive a packet */

	ret = 0;
exit:
	/*kill qemu's*/
	if (qemu1_pid)
		pg_util_stop_qemu(qemu1_pid, SIGQUIT);

	if (qemu2_pid)
		pg_util_stop_qemu(qemu2_pid, SIGQUIT);

	/*Free all*/
	if (pkts) {
		pg_packets_free(pkts, 1);
		g_free(pkts);
	}
	pg_graph_destroy(graph1);
	pg_graph_destroy(graph);
	pg_graph_destroy(graph3);
	pg_vhost_stop();
	g_assert(!ret);
}

static void test_graph_firewall_intense(void)
{
	struct pg_brick *nic, *vtep, *print, *vhost, *firewall;
	struct pg_error *error = NULL;
	struct ether_addr mac_vtep = {{0xb0, 0xb1, 0xb2,
				       0xb3, 0xb4, 0xb5} };
	int ret = -1;
	struct pg_graph *graph = NULL;

	pg_vhost_start("/tmp", &error);
	CHECK_ERROR(error);
	g_assert(!pg_init_seccomp());

	for (int i = 0; i < 100; ++i) {
		nic = pg_nic_new_by_id("nic", ring_port(), &error);
		CHECK_ERROR(error);
		vtep = pg_vtep_new("vt", 50, PG_WEST_SIDE,
				    0x000000EE, mac_vtep, PG_VTEP_DST_PORT,
					PG_VTEP_ALL_OPTI, &error);
		CHECK_ERROR(error);
		print = pg_print_new("main-print", NULL, PG_PRINT_FLAG_MAX,
							 NULL, &error);
		CHECK_ERROR(error);
		firewall = pg_firewall_new("firewall",
						   PG_NO_CONN_WORKER, &error);
		CHECK_ERROR(error);
		vhost = pg_vhost_new("vhost", 0, &error);
		CHECK_ERROR(error);
		pg_brick_chained_links(&error, nic, print, vtep,
					   firewall, vhost);
		CHECK_ERROR(error);
		/* Created graph */
		graph = pg_graph_new("graph_1", nic, &error);
		CHECK_ERROR(error);
		/* Add firewall rule */
		ASSERT(pg_firewall_rule_add(firewall, "icmp",
						PG_MAX_SIDE, 1, &error) == 0);
		CHECK_ERROR_ASSERT(error);
		pg_graph_destroy(graph);
	}

	CHECK_ERROR_ASSERT(error);

	ret = 0;
exit:
	pg_vhost_stop();
	g_assert(!ret);
}

#define PG_BRANCHES_NB 64

static void test_graph_firewall_intense_multiple(void)
{
	struct pg_error *error = NULL;
	struct branch branches[PG_BRANCHES_NB];
	struct pg_graph *graphs[PG_BRANCHES_NB];
	GString *tmp = g_string_new(NULL);
	int ret = -1;

	pg_vhost_start("/tmp", &error);
	CHECK_ERROR(error);
	g_assert(!pg_init_seccomp());

	for (int i = 0; i < 100; ++i) {
		for (int j = 0; j < PG_BRANCHES_NB; j++) {

			/* Create graph brick */
			g_string_printf(tmp, "fw-%d%d", i, j);
			branches[j].firewall = pg_firewall_new(tmp->str,
						   PG_NO_CONN_WORKER, &error);
			CHECK_ERROR(error);
			g_string_printf(tmp, "vhost-%d%d", i, j);
			branches[j].vhost = pg_vhost_new(tmp->str, 0, &error);
			CHECK_ERROR(error);

			/* Link brick */
			pg_brick_chained_links(&error, branches[j].firewall,
							   branches[j].vhost);
			CHECK_ERROR(error);

			/* create graph */
			g_string_printf(tmp, "graph-%d", j);
			graphs[j] = pg_graph_new(tmp->str,
						 branches[j].firewall, &error);
			CHECK_ERROR(error);

			/* Add firewall rule */
			ASSERT(pg_firewall_rule_add(branches[j].firewall,
						    "icmp", PG_MAX_SIDE,
						    1, &error) == 0);
			CHECK_ERROR(error);
		}
		for (int j = 0; j < PG_BRANCHES_NB; j++)
			pg_graph_destroy(graphs[j]);
	}

	CHECK_ERROR_ASSERT(error);

	ret = 0;
exit:
	g_string_free(tmp, 1);
	pg_vhost_stop();
	g_assert(!ret);
}

int main(int argc, char **argv)
{
	int r, test_flags;

	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	r = pg_start(argc, argv);
	g_assert(r >= 0);

	r += + 1;
	argc -= r;
	argv += r;
	test_flags = parse_args(argc, argv);
	if ((test_flags & (VM | VM_KEY | HUGEPAGES)) == 0)
		test_flags |= PRINT_USAGE;
	if (test_flags & PRINT_USAGE)
		print_usage();
	g_assert(!(test_flags & FAIL));
	pg_test_add_func("/integration/graph/flow",
			test_graph_type1);
	pg_test_add_func("/integration/graph/intense/solo",
			test_graph_firewall_intense);
	pg_test_add_func("/integration/graph/intense/multiple",
			test_graph_firewall_intense_multiple);

	r = g_test_run();

	pg_stop();
	return r;
}
