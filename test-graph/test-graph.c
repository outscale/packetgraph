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

#include <packetgraph/common.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/config.h>
#include <packetgraph/utils/errors.h>
#include <packetgraph/utils/mac.h>
#include <packetgraph/utils/qemu.h>
#include <packetgraph/packetgraph.h>
#include <packetgraph/packets.h>
#include <packetgraph/brick.h>
#include <packetgraph/collect.h>
#include <packetgraph/nic.h>
#include <packetgraph/vtep.h>
#include <packetgraph/vhost.h>
#include <packetgraph/print.h>
#include <packetgraph/firewall.h>
#include <packetgraph/antispoof.h>

enum test_flags {
	PRINT_USAGE = 1,
	FAIL = 2,
	CPIO = 4,
	BZIMAGE = 8,
	HUGEPAGES = 16
};

#define VNI_1	1
#define VNI_2	2

static char *glob_bzimage_path;
static char *glob_cpio_path;
static char *glob_hugepages_path;

#define PG_GC_INIT(name)			\
	GList *name = NULL			\


#define PG_GC_ADD(name, brick)				\
	(name = g_list_append(name, brick))

static inline void pg_gc_chained_add_int(GList *name, ...)
{
	va_list ap;
	struct pg_brick *cur;
	
	va_start(ap, name);
	while ((cur = va_arg(ap, struct pg_brick *)) != NULL)
		PG_GC_ADD(name, cur);
	va_end(ap);
}

#define PG_GC_CHAINED_ADD(name, args...)		\
	(pg_gc_chained_add_int(name, args, NULL))


static void pg_brick_destroy_wraper(void *arg)
{
	pg_brick_destroy(arg);
}

#define PG_GC_DESTROY(name) do {					\
		g_list_free_full(name, pg_brick_destroy_wraper);	\
	} while (0)


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
	printf("tests usage: [EAL options] -- [-help] -bzimage /path/to/kernel -cpio /path/to/rootfs.cpio -hugepages /path/to/hugepages/mount\n");
	exit(0);
}

static uint64_t parse_args(int argc, char **argv)
{
	int i;
	int ret = 0;

	for (i = 0; i < argc; ++i) {
		if (!strcmp("-help", argv[i])) {
			ret |= PRINT_USAGE;
		} else if (!strcmp("-bzimage", argv[i]) && i + 1 < argc) {
			glob_bzimage_path = argv[i + 1];
			ret |= BZIMAGE;
			i++;
		} else if (!strcmp("-cpio", argv[i]) && i + 1 < argc) {
			glob_cpio_path = argv[i + 1];
			ret |= CPIO;
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

static void test_graph_type1(void)
{
	struct pg_brick *vhost1, *vhost2, *antispoof1, *antispoof2, *fw1, *fw2;
	struct pg_brick *nic, *vtep, *print;
	struct pg_brick *vhost1_reader, *vhost2_reader, *collect1, *collect2;
	struct pg_error *error = NULL;
	struct ether_addr mac_vtep = {{0xb0, 0xb1, 0xb2,
				       0xb3, 0xb4, 0xb5}};
	struct ether_addr mac1 = {{0x52,0x54,0x00,0x12,0x34,0x11}};
	struct ether_addr mac2 = {{0x52,0x54,0x00,0x12,0x34,0x21}};
	const char mac_reader_1[18] = "52:54:00:12:34:12";
	const char mac_reader_2[18] = "52:54:00:12:34:22";
	char tmp_mac1[20];
	char tmp_mac2[20];
	const char *socket_path1, *socket_output_path1;
	const char *socket_path2, *socket_output_path2;
	int qemu1_pid = 0;
	int qemu2_pid = 0;
	struct rte_mbuf **pkts = NULL;
	struct rte_mbuf **result_pkts;
	uint16_t count;
	uint64_t pkts_mask = 0;
	int ret = -1;
	int exit_status;
	uint32_t len;

	PG_GC_INIT(brick_gc);

	pg_vhost_start("/tmp", &error);
	CHECK_ERROR(error);
	
	nic = pg_nic_new_by_id("nic", 1, 1, WEST_SIDE, ring_port(), &error);
	CHECK_ERROR(error);
	vtep = pg_vtep_new("vt", 1, 50, WEST_SIDE,
			   0x000000EE, mac_vtep, ALL_OPTI, &error);
	CHECK_ERROR(error);
	fw1 = pg_firewall_new("fw2", 1, 1, PG_NO_CONN_WORKER, &error);
	CHECK_ERROR(error);
	fw2 = pg_firewall_new("fw2", 1, 1, PG_NO_CONN_WORKER, &error);
	CHECK_ERROR(error);
	antispoof1 = pg_antispoof_new("antispoof1", 1, 1, EAST_SIDE,
				     mac1, &error);
	CHECK_ERROR(error);
	antispoof2 = pg_antispoof_new("antispoof2", 1, 1, EAST_SIDE,
				     mac2, &error);
	CHECK_ERROR(error);
	vhost1 = pg_vhost_new("vhost-1", 1, 1, EAST_SIDE, &error);
	CHECK_ERROR(error);
	vhost2 = pg_vhost_new("vhost-2", 1, 1, EAST_SIDE, &error);
	CHECK_ERROR(error);

	vhost1_reader = pg_vhost_new("vhost-1-reader", 1, 1, WEST_SIDE, &error);
	CHECK_ERROR(error);
	vhost2_reader = pg_vhost_new("vhost-2-reader", 1, 1, WEST_SIDE, &error);
	CHECK_ERROR(error);

	collect1 = pg_collect_new("collect-1-reader", 1, 1, &error);
	CHECK_ERROR(error);
	collect2 = pg_collect_new("collect-2-reader", 1, 1, &error);
	CHECK_ERROR(error);

	print = pg_print_new("print", 1, 1, NULL, PG_PRINT_FLAG_MAX,
			     NULL, &error);
	CHECK_ERROR(error);
	
	PG_GC_CHAINED_ADD(brick_gc, vhost1, vhost2, antispoof1,
			  antispoof2, fw1, fw2, nic, vtep, print,
			  vhost1_reader, vhost2_reader, collect1, collect2);
	/*
	 * Our main graph:
	 *	                FIREWALL1 -- ANTISPOOF1 -- VHOST1
	 *		      /
	 * NIC -- PRINT-- VTEP
	 *	              \ FIREWALL2 -- ANTISPOOF2 -- VHOST2
	 */
	/* For now, we're not going to use antispoof ...*/
	pg_brick_chained_links(&error, nic, print, vtep, fw1, vhost1);
	CHECK_ERROR(error);
	pg_brick_chained_links(&error, vtep, fw2, vhost2);
	CHECK_ERROR(error);

	/*
	 * graph use to read and write on vhost1:
	 * 
	 * VHOST1_READER -- COLLECT1
	 */
	pg_brick_chained_links(&error, vhost1_reader, collect1);
	CHECK_ERROR(error);

	/*
	 * graph use to read and write on vhost2:
	 *
	 * VHOST2_READER -- COLLECT2
	 */
	pg_brick_chained_links(&error, vhost2_reader, collect2);
	CHECK_ERROR(error);

	/* Get socket paths */
	socket_path1 = pg_brick_handle_dup(vhost1, &error);
	g_assert(!error);
	socket_output_path1 = pg_brick_handle_dup(vhost1_reader, &error);
	g_assert(!error);
	socket_path2 = pg_brick_handle_dup(vhost2, &error);
	g_assert(!error);
	socket_output_path2 = pg_brick_handle_dup(vhost2_reader, &error);
	g_assert(!error);

	/* Translate MAC to strings */
	g_assert(pg_printable_mac(&mac1, tmp_mac1));
	g_assert(pg_printable_mac(&mac2, tmp_mac2));


	g_assert(g_file_test(socket_path1, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(socket_output_path1, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_bzimage_path, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_cpio_path, G_FILE_TEST_EXISTS));
	g_assert(g_file_test(glob_hugepages_path, G_FILE_TEST_EXISTS));
	/* spawm time ! */
	qemu1_pid = pg_spawn_qemu(socket_path1, socket_output_path1,
				  tmp_mac1, mac_reader_1, glob_bzimage_path,
				  glob_cpio_path, glob_hugepages_path, &error);
	CHECK_ERROR_ASSERT(error);
	g_assert(qemu1_pid);
	printf("qemu1 has been started\n");
	qemu2_pid = pg_spawn_qemu(socket_path2, socket_output_path2,
				  tmp_mac2, mac_reader_2, glob_bzimage_path,
				  glob_cpio_path, glob_hugepages_path, &error);
	CHECK_ERROR_ASSERT(error);
	g_assert(qemu2_pid);
	printf("qemu2 has been started\n");

	/* Now we need to kill qemu before exit in case of error */
	
	/* Add VNI's */
	pg_vtep_add_vni(vtep, fw1, VNI_1,
			inet_addr("225.0.0.1"), &error);
	CHECK_ERROR_ASSERT(error);
	pg_vtep_add_vni(vtep, fw2, VNI_2,
			inet_addr("225.0.0.2"), &error);
	CHECK_ERROR_ASSERT(error);

	/* Add firewall rule */
	ASSERT(!pg_firewall_rule_add(fw1, "icmp", MAX_SIDE, 1, &error));
	ASSERT(!pg_firewall_rule_add(fw2, "icmp", MAX_SIDE, 1, &error));

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
	ASSERT(pg_brick_pkts_count_get(fw1, EAST_SIDE));
	/* ASSERT(pg_brick_pkts_count_get(antispoof1, EAST_SIDE)); */
	ASSERT(pg_brick_pkts_count_get(vhost1, EAST_SIDE));
	
	/* check the collect1 */
	for (int i = 0; i < 10; i++) {
		usleep(100000);
		pg_brick_poll(vhost1_reader, &count, &error);
		g_assert(!error);
		if (count)
			break;
	}

	CHECK_ERROR_ASSERT(error);
	ASSERT(count);
	/* same with VNI 2 */
	result_pkts = pg_brick_west_burst_get(collect1, &pkts_mask, &error);
	ASSERT(result_pkts);

	/* Write in vhost2_reader */
	/* Check all step of the transmition (again in the oposit way) */
	/* Check the ring has recive a packet */

	ret = 0;
exit:
	/*kill qemu's*/
	if (qemu1_pid) {
		kill(qemu1_pid, SIGKILL);
		waitpid(qemu1_pid, &exit_status, 0);
		g_spawn_close_pid(qemu1_pid);
	}

	if (qemu2_pid) {
		kill(qemu2_pid, SIGKILL);
		waitpid(qemu2_pid, &exit_status, 0);
		g_spawn_close_pid(qemu2_pid);
	}

	/*Free all*/
	if (pkts) {
		pg_packets_free(pkts, 1);
		g_free(pkts);
	}
	PG_GC_DESTROY(brick_gc);
	pg_vhost_stop();
	g_assert(!ret);
}

int main(int argc, char **argv)
{
	struct pg_error *error;
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
	if ((test_flags & (BZIMAGE | CPIO | HUGEPAGES)) == 0)
		test_flags |= PRINT_USAGE;
	if (test_flags & PRINT_USAGE)
		print_usage();
	g_assert(!(test_flags & FAIL));

	g_test_add_func("/brick/graph/type1",
			test_graph_type1);
	r = g_test_run();

	pg_stop();
	return r;
}
