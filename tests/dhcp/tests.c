 /* Copyright 2017
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
#include <string.h>

#define __FAVOR_BSD
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#include <rte_config.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <pcap/pcap.h>

#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include <packetgraph/print.h>
#include <packetgraph/dhcp.h>
#include "brick-int.h"
#include "collect.h"
#include "fail.h"
#include "packetsgen.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/qemu.h"
#include "utils/mac.h"
#include "utils/bitmask.h"

extern char * ether_ntoa(struct ether_addr *e);
static uint16_t ssh_port_id = 65000;

struct dhcp_messages_payload {
        uint8_t op;
        uint8_t htype;
        uint8_t hlen;
        uint8_t hops;
        uint32_t xid;
        uint16_t secs;
        uint16_t flags;
        uint32_t ciaddr;
        uint32_t yiaddr;
        uint32_t siaddr;
        uint32_t giaddr;
        struct ether_addr mac_client;
        char * server_name;
        char * file;
        uint8_t dhcp_m_type;
        struct ether_addr client_id;
        uint32_t subnet_mask;
        uint32_t request_ip;
        uint32_t server_identifier;
        uint32_t router_ip;
        int lease_time;
        uint8_t *options;
        GList *request_parameters;
};

struct pg_dhcp_state {
        struct pg_brick brick;
        struct ether_addr **mac;
        in_addr_t addr_net;
        in_addr_t addr_broad;
        int *check_ip;
	int prefix;
};

static void test_dhcp_discover(void)
{
	struct pg_brick *dhcp;
	struct pg_brick *collect;
	struct ether_addr eth_dst;
	struct ether_addr eth_src;
	struct ether_addr eth_dhcp;
	struct rte_mbuf **pkts;
	struct rte_mbuf **sended_pkts;
	struct dhcp_messages_payload dhcp_hdr;
	char *tmp;
	uint64_t nb_packets = pg_mask_firsts(1);
	uint8_t dhcp_mes_type = 2;
	struct pg_error *error = NULL;

        pg_scan_ether_addr(&eth_dst, "FF:FF:FF:FF:FF:FF");
        pg_scan_ether_addr(&eth_src, "00:18:b9:56:2e:73");
        pg_scan_ether_addr(&eth_dhcp, "44:82:c9:2b:a2:37");
        pkts = pg_packets_append_ether(pg_packets_create(nb_packets),
                                       nb_packets, &eth_src, &eth_dst,
                                       ETHER_TYPE_IPv4);
        pg_packets_append_ipv4(pkts, nb_packets, 0x00000000, 0xFFFFFFFF,
                                sizeof(struct ipv4_hdr), 17);

	pg_packets_append_udp(pkts, nb_packets, 68, 67, sizeof(struct udp_hdr));

        PG_FOREACH_BIT(nb_packets, j) {
        if (!pkts[j])
                continue;
        dhcp_hdr.mac_client = eth_src;
        dhcp_hdr.dhcp_m_type = 1;
        dhcp_hdr.client_id = eth_src;
        tmp = rte_pktmbuf_append(pkts[j], sizeof(struct dhcp_messages_payload));
        if (!tmp)
                printf("error buffer\n");
        memcpy(tmp, &dhcp_hdr, sizeof(struct dhcp_messages_payload));
        }

	collect = pg_collect_new("collect", &error);
	g_assert(!error);

        dhcp = pg_dhcp_new("dhcp", "192.54.30.200/24", eth_dhcp, &error);
        g_assert(!error);

        pg_brick_link(dhcp, collect, &error);
        g_assert(!error);

	pg_brick_burst_to_east(dhcp, 0, pkts, nb_packets, &error);
	g_assert(!error);

	sended_pkts = pg_brick_west_burst_get(collect, &nb_packets, &error);
	g_assert(!error);

	PG_FOREACH_BIT(nb_packets, j) {
	if (!pkts[j])
                continue;
	uint32_t hdrs_len = sizeof(struct ether_hdr) +
			    sizeof(struct ipv4_hdr) +
			    sizeof(struct udp_hdr);
	struct dhcp_messages_payload *dhcp_hdr_sended =
		(struct dhcp_messages_payload *)
		rte_pktmbuf_mtod_offset(sended_pkts[j], char *, hdrs_len);
	g_assert(dhcp_hdr_sended->dhcp_m_type == dhcp_mes_type);	
	}
	
	pg_brick_destroy(collect);
	pg_brick_destroy(dhcp);
	pg_packets_free(pkts, pg_mask_firsts(1));
	g_free(pkts);
}

static void test_dhcp_packets_registering(void)
{
	struct pg_brick *dhcp;
	struct pg_brick *packets_gen;
	struct rte_mbuf **pkts;
	struct pg_error *error = NULL;
	struct ether_addr eth_dst;
	struct ether_addr eth_src;
	struct ether_addr eth_dhcp;
	struct dhcp_messages_payload dhcp_hdr;
	char *tmp;
	uint16_t nb_packets = 1;

	pg_scan_ether_addr(&eth_dst, "FF:FF:FF:FF:FF:FF");
	pg_scan_ether_addr(&eth_src, "00:18:b9:56:2e:73");
	pg_scan_ether_addr(&eth_dhcp, "44:82:c9:2b:a2:37");
        pkts = pg_packets_append_ether(pg_packets_create(pg_mask_firsts(1)),
                                       pg_mask_firsts(1), &eth_src, &eth_dst,
                                       ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, pg_mask_firsts(1), 0x00000000, 0xFFFFFFFF,
				sizeof(struct ipv4_hdr), 17);

	pg_packets_append_udp(pkts, pg_mask_firsts(1), 68, 67, sizeof(struct udp_hdr));

	PG_FOREACH_BIT(pg_mask_firsts(1), j) {
	if (!pkts[j])
		continue;
	dhcp_hdr.mac_client = eth_src;
	dhcp_hdr.dhcp_m_type = 3;
	dhcp_hdr.client_id = eth_src;
	dhcp_hdr.request_ip = 0xC0361E01;
	tmp = rte_pktmbuf_append(pkts[j], sizeof(struct dhcp_messages_payload));
	if (!tmp)
		printf("error buffer\n");
	memcpy(tmp, &dhcp_hdr, sizeof(struct dhcp_messages_payload));
	}

	packets_gen = pg_packetsgen_new("packetsgen", 1, 1, PG_EAST_SIDE, pkts,
					1, &error);
	g_assert(!error);

	dhcp = pg_dhcp_new("dhcp", "192.54.30.200/24", eth_dhcp, &error);
	g_assert(!error);

	pg_brick_link(packets_gen, dhcp, &error);
	g_assert(!error);

	pg_brick_poll(packets_gen, &nb_packets, &error);
	g_assert(!error);

	struct pg_dhcp_state *state = pg_brick_get_state(dhcp,
                                                        struct pg_dhcp_state);
        int size = sizeof(state->mac) / sizeof(struct ether_addr);
        for(int i = 1; i <= size ; i++) {
		printf("%i \n", i);
		if (state->mac[i]) {
                	printf("%s\n", ether_ntoa(state->mac[i]));
		}
		else {
			printf("No Mac assigned\n");
		}
	}

        pg_brick_destroy(packets_gen);
        pg_brick_destroy(dhcp);
        pg_packets_free(pkts, pg_mask_firsts(1));
        g_free(pkts);

}
/*
static void test_adressing_vm(void)
{
	struct pg_brick *dhcp, *vhost, *print, *collect;
	const char *socket_path;
	struct ether_addr mac_vm;
	struct ether_addr mac_dhcp;
	pg_scan_ether_addr(&mac_vm, "42:18:b9:56:2e:73");
	pg_scan_ether_addr(&mac_dhcp, "55:27:c9:ea:9d:36");
	struct pg_error *error = NULL;
	int ret, qemu_pid;
       
        ret = pg_vhost_start("/tmp", &error);
        g_assert(ret == 0);
        g_assert(!error);

        vhost = pg_vhost_new("vhost", PG_VHOST_USER_DEQUEUE_ZERO_COPY,
                               &error);
        g_assert(!error);
        g_assert(vhost);

	dhcp = pg_dhcp_new("dhcp", "192.54.30.200/24", mac_dhcp, &error);
	g_assert(!error);
	g_assert(dhcp);

	collect = pg_collect_new("collect", &error);
	g_assert(!error);
	g_assert(collect);

        print = pg_print_new("My print", NULL, PG_PRINT_FLAG_MAX, NULL,
                             &error);
	g_assert(!error);
	g_assert(print);

	pg_brick_link(collect, vhost, &error);
	g_assert(!error);

	socket_path = pg_vhost_socket_path(vhost, &error);
	g_assert(!error);
	g_assert(socket_path);

	qemu_pid = pg_util_spawn_qemu(socket_path, NULL, mac_vm, NULL,
				      glob_vm_path, glob_vm_key_path,
				      glob_hugepages_path, &error);

	g_assert(!error);
	g_assert(qemu_pid);

#       define SSH(c) \
                g_assert(pg_util_ssh("localhost", ssh_port_id, \
                glob_vm_key_path, c) == 0)
	SSH("dhclient ens4");
#       undef SSH

	pg_util_stop_qemu(qemu_pid, qemu_exit_signal);

}
*/
static void test_dhcp(void)
{
        pg_test_add_func("/dhcp/discover\n",
                         test_dhcp_discover);
	pg_test_add_func("/dhcp/packets_registering\n",
			 test_dhcp_packets_registering);
}

int main(int argc, char **argv)
{
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	g_assert(pg_start(argc, argv) >= 0);

	test_dhcp();
	int r = g_test_run();

	pg_stop();
	return r;
}
