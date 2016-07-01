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
#include <packetgraph/common.h>
#include <packetgraph/nic.h>
#include <packetgraph/vtep.h>
#include <packetgraph/vhost.h>
#include <packetgraph/print.h>
#include <packetgraph/firewall.h>
#include <packetgraph/antispoof.h>
#include <packetgraph/errors.h>
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

static inline void pg_gc_chained_add_int(GList **name, ...)
{
	va_list ap;
	struct pg_brick *cur;

	va_start(ap, name);
	while ((cur = va_arg(ap, struct pg_brick *)) != NULL)
		*name = g_list_append(*name, cur);
	va_end(ap);
}

#define PG_GC_CHAINED_ADD(name, args...)		\
	(pg_gc_chained_add_int(&name, args, NULL))


static void pg_brick_destroy_wraper(void *arg, void *useless)
{
	pg_brick_destroy(arg);
}

static void pg_gc_destroy(GList *graph)
{
	g_list_foreach(graph, pg_brick_destroy_wraper, NULL);
	g_list_free(graph);
}


#define ASSERT(check) do {						\
		if (!(check)) {						\
			dprintf(2, "line=%d 'function=%s': assertion fail\n", \
				__LINE__, __FUNCTION__ );		\
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
	printf("tests usage: [EAL options] -- [-help] -vm /path/to/vm/image -vm-key /path/to/vm/key -hugepages /path/to/hugepages/mount\n");
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
	r1 = rte_ring_create("R1", 256, 0,RING_F_SP_ENQ|RING_F_SC_DEQ);
	port1 = rte_eth_from_rings("r1-port", &r1, 1, &r1, 1, 0);
	return port1;
}

static void rm_graph_branch(struct branch *branch)
{
	pg_gc_destroy(branch->collector);
}

static inline const char *sock_path_graph(struct branch *branch,
					  struct pg_error **errp)
{
	return pg_vhost_socket_path(branch->vhost, errp);
}

static inline const char *sock_read_path_graph(struct branch *branch,
					       struct pg_error **errp)
{
	return pg_vhost_socket_path(branch->vhost_reader, errp);
}

static inline int start_qemu_graph(struct branch *branch,
				  const char *mac_reader,
				  struct pg_error **errp)
{
	static uint32_t ssh_port_id = 65000;
	char tmp_mac[20];

	g_assert(pg_printable_mac(&branch->mac, tmp_mac));
	int qemu_pid = pg_util_spawn_qemu(sock_path_graph(branch, errp),
					  sock_read_path_graph(branch, errp),
					  tmp_mac, mac_reader,
					  glob_vm_path,
					  glob_vm_key_path,
					  glob_hugepages_path, errp);


#	define SSH(c) \
	g_assert(pg_util_ssh("localhost", ssh_port_id, glob_vm_key_path, c) == 0)
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

	g_string_printf(tmp, "antispoof-%d", id);
	branch->antispoof = pg_antispoof_new(tmp->str,
					     1, 1, EAST_SIDE,
					     &mac, &error);
	CHECK_ERROR(error);

	g_string_printf(tmp, "vhost-%d", id);
	branch->vhost = pg_vhost_new(tmp->str, 1, 1, EAST_SIDE, &error);
	CHECK_ERROR(error);

	g_string_printf(tmp, "vhost-reader-%d", id);
	branch->vhost_reader = pg_vhost_new(tmp->str, 1, 1, WEST_SIDE,
					    &error);
	CHECK_ERROR(error);

	g_string_printf(tmp, "collect-reader-%d", id);
	branch->collect = pg_collect_new(tmp->str, &error);
	CHECK_ERROR(error);

	g_string_printf(tmp, "print-%d", id);
	branch->print = pg_print_new(tmp->str, NULL, PG_PRINT_FLAG_MAX,
				     NULL, &error);
	CHECK_ERROR(error);

	PG_GC_CHAINED_ADD(branch->collector, branch->firewall, branch->antispoof,
			  branch->vhost, branch->vhost_reader, branch->collect,
			  branch->print);

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
				       0xb3, 0xb4, 0xb5}};
	struct ether_addr mac1 = {{0x52,0x54,0x00,0x12,0x34,0x11}};
	struct ether_addr mac2 = {{0x52,0x54,0x00,0x12,0x34,0x21}};
	const char mac_reader_1[18] = "52:54:00:12:34:12";
	const char mac_reader_2[18] = "52:54:00:12:34:22";
	struct branch branch1, branch2;
	int qemu1_pid = 0;
	int qemu2_pid = 0;
	struct rte_mbuf **pkts = NULL;
	struct rte_mbuf **result_pkts;
	uint16_t count;
	uint64_t pkts_mask = 0;
	int ret = -1;
	uint32_t len;
	GList *brick_gc = NULL;

	pg_vhost_start("/tmp", &error);
	CHECK_ERROR(error);

	nic = pg_nic_new_by_id("nic", ring_port(), &error);
	if (error) pg_error_print(error);
	CHECK_ERROR(error);
	vtep = pg_vtep_new("vt", 1, 50, WEST_SIDE,
			   0x000000EE, mac_vtep, ALL_OPTI, &error);
	CHECK_ERROR(error);

	print = pg_print_new("print", NULL, PG_PRINT_FLAG_MAX,
			     NULL, &error);
	CHECK_ERROR(error);

	PG_GC_CHAINED_ADD(brick_gc, nic, vtep, print);

	g_assert(add_graph_branch(&branch1, 1, mac1, 0, 0));
	g_assert(add_graph_branch(&branch2, 2, mac2, 0, 0));

	/*
	 * Our main graph:
	 *	                FIREWALL1 -- ANTISPOOF1 -- VHOST1
	 *		      /
	 * NIC -- PRINT-- VTEP
	 *	              \ FIREWALL2 -- ANTISPOOF2 -- VHOST2
	 */
	pg_brick_chained_links(&error, nic, print, vtep);
	CHECK_ERROR(error);

	g_assert(link_graph_branch(&branch1, vtep));
	g_assert(link_graph_branch(&branch2, vtep));

	/* Translate MAC to strings */

	g_assert(g_file_test(sock_path_graph(&branch1, &error),
			     G_FILE_TEST_EXISTS));
	g_assert(g_file_test(sock_read_path_graph(&branch1, &error),
			     G_FILE_TEST_EXISTS));
	g_assert(g_file_test(sock_path_graph(&branch2, &error),
			     G_FILE_TEST_EXISTS));
	g_assert(g_file_test(sock_read_path_graph(&branch2, &error),
			     G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_vm_path, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_vm_key_path, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_hugepages_path, G_FILE_TEST_EXISTS));
	/* spawm time ! */
	qemu1_pid = start_qemu_graph(&branch1, mac_reader_1,  &error);
	CHECK_ERROR_ASSERT(error);
	g_assert(qemu1_pid);
	qemu2_pid = start_qemu_graph(&branch2, mac_reader_2,  &error);
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
				     MAX_SIDE, 1, &error) == 0);
	ASSERT(pg_firewall_rule_add(branch2.firewall, "icmp",
				     MAX_SIDE, 1, &error) == 0);

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
			      1, 152, 153, len - sizeof(struct ipv4_hdr));
	pg_packets_append_vxlan(pkts, 1, VNI_1);
	pg_packets_append_ether(pkts, 1, &mac2, &mac_vtep, 4);
	pg_packets_append_str(pkts, 1, "hello :)");

	/* Write the packet into the ring */
	rte_eth_tx_burst(ring_port(), 0, pkts, 1);
	pg_brick_poll(nic, &count, &error);

	CHECK_ERROR_ASSERT(error);
	ASSERT(count);

	/* Check all step of the transmition :) */
	ASSERT(pg_brick_pkts_count_get(nic, WEST_SIDE));
	ASSERT(pg_brick_pkts_count_get(vtep, EAST_SIDE));
	ASSERT(pg_brick_pkts_count_get(branch1.firewall, EAST_SIDE));
	ASSERT(pg_brick_pkts_count_get(branch1.vhost, EAST_SIDE));

	/* check the collect1 */
	for (int i = 0; i < 10; i++) {
		usleep(100000);
		pg_brick_poll(branch1.vhost_reader, &count, &error);
		CHECK_ERROR_ASSERT(error);
		if (count)
			break;
	}

	CHECK_ERROR_ASSERT(error);
	ASSERT(pg_brick_pkts_count_get(branch1.collect,
				       EAST_SIDE));
	ASSERT(count);
	/* same with VNI 2 */
	result_pkts = pg_brick_west_burst_get(branch1.collect, &pkts_mask,
					      &error);
	ASSERT(result_pkts);

	/* Write in vhost2_reader */
	/* Check all step of the transmition (again in the oposit way) */
	/* Check the ring has recive a packet */

	ret = 0;
exit:
	/*kill qemu's*/
	if (qemu1_pid) {
		pg_util_stop_qemu(qemu1_pid);
	}

	if (qemu2_pid) {
		pg_util_stop_qemu(qemu2_pid);
	}

	/*Free all*/
	if (pkts) {
		pg_packets_free(pkts, 1);
		g_free(pkts);
	}
	rm_graph_branch(&branch1);
	rm_graph_branch(&branch2);
	pg_gc_destroy(brick_gc);
	pg_vhost_stop();
	g_assert(!ret);
}

static void test_graph_firewall_intense(void)
{
	struct pg_brick *nic, *vtep, *print;
	struct pg_error *error = NULL;
	struct ether_addr mac_vtep = {{0xb0, 0xb1, 0xb2,
				       0xb3, 0xb4, 0xb5}};
	struct ether_addr mac1 = {{0x52,0x54,0x00,0x12,0x34,0x11}};
	struct branch branch1;
	int ret = -1;
	GList *brick_gc = NULL;

	pg_vhost_start("/tmp", &error);
	CHECK_ERROR(error);

	nic = pg_nic_new_by_id("nic", ring_port(), &error);
	CHECK_ERROR(error);
	vtep = pg_vtep_new("vt", 1, 50, WEST_SIDE,
			   0x000000EE, mac_vtep, ALL_OPTI, &error);
	CHECK_ERROR(error);

	print = pg_print_new("main-print", NULL, PG_PRINT_FLAG_MAX,
			     NULL, &error);
	CHECK_ERROR(error);
	PG_GC_CHAINED_ADD(brick_gc, nic, vtep, print);

	pg_brick_chained_links(&error, nic, print, vtep);
	CHECK_ERROR(error);

	for (int i = 0; i < 100; ++i) {
		g_assert(add_graph_branch(&branch1, 1, mac1, 0, 0));
		g_assert(link_graph_branch(&branch1, vtep));

		/* Add firewall rule */
		ASSERT(pg_firewall_rule_add(branch1.firewall, "icmp",
					     MAX_SIDE, 1, &error) == 0);
		rm_graph_branch(&branch1);
	}

	CHECK_ERROR_ASSERT(error);

	ret = 0;
exit:
	pg_gc_destroy(brick_gc);
	pg_vhost_stop();
	g_assert(!ret);
}

static void test_graph_firewall_intense_multiple(void)
{
	struct pg_brick *nic, *vtep, *print;
	struct pg_error *error = NULL;
	struct ether_addr mac_vtep = {{0xb0, 0xb1, 0xb2,
				       0xb3, 0xb4, 0xb5}};
	struct ether_addr mac1 = {{0x52,0x54,0x00,0x12,0x34,0x11}};
	int branches_nb = 64;
	struct branch branches[branches_nb];
	int ret = -1;
	GList *brick_gc = NULL;

	pg_vhost_start("/tmp", &error);
	CHECK_ERROR(error);

	nic = pg_nic_new_by_id("nic", ring_port(), &error);
	CHECK_ERROR(error);
	vtep = pg_vtep_new("vt", 1, branches_nb, WEST_SIDE,
			   0x000000EE, mac_vtep, ALL_OPTI, &error);
	CHECK_ERROR(error);

	print = pg_print_new("main-print", NULL, PG_PRINT_FLAG_MAX,
			     NULL, &error);
	CHECK_ERROR(error);
	PG_GC_CHAINED_ADD(brick_gc, nic, vtep, print);

	pg_brick_chained_links(&error, nic, print, vtep);
	CHECK_ERROR(error);

	for (int i = 0; i < 100; ++i) {
		for (int j = 0; j < branches_nb; j++) {
			g_assert(add_graph_branch(&branches[j], j, mac1, 0, 0));
			g_assert(link_graph_branch(&branches[j], vtep));

			/* Add firewall rule */
			ASSERT(pg_firewall_rule_add(branches[j].firewall, "icmp",
						     MAX_SIDE, 1, &error) == 0);
		}
		for (int j = 0; j < branches_nb; j++) {
			rm_graph_branch(&branches[j]);
		}
	}

	CHECK_ERROR_ASSERT(error);

	ret = 0;
exit:
	pg_gc_destroy(brick_gc);
	pg_vhost_stop();
	g_assert(!ret);
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int r, test_flags;

	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	r = pg_start(argc, argv, &error);
	g_assert(!error);

	r += + 1;
	argc -= r;
	argv += r;
	test_flags = parse_args(argc, argv);
	if ((test_flags & (VM | VM_KEY | HUGEPAGES)) == 0)
		test_flags |= PRINT_USAGE;
	if (test_flags & PRINT_USAGE)
		print_usage();
	g_assert(!(test_flags & FAIL));
	g_test_add_func("/integration/graph/flow",
			test_graph_type1);
	g_test_add_func("/integration/graph/intense/solo",
			test_graph_firewall_intense);
	g_test_add_func("/integration/graph/intense/multiple",
			test_graph_firewall_intense_multiple);

	r = g_test_run();

	pg_stop();
	return r;
}
