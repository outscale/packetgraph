/* Copyright 2017 Outscale SAS
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
#include <rte_ether.h>
#include <pcap/pcap.h>
#include <endian.h>

#include <packetgraph/packetgraph.h>
#include "utils/bitmask.h"
#include "utils/mempool.h"
#include "packets.h"
#include "brick-int.h"
#include "utils/network.h"
#include "src/npf/npf/dpdk/npf_dpdk.h"

#define DHCP_SIDE_TO_NPF(side) \
        ((side) == PG_WEST_SIDE ? PFIL_OUT : PFIL_IN)

bool eth_compare(uint8_t eth1[6], uint8_t eth2[6]);

struct dhcp_messages_payload {
        uint8_t op;
        uint8_t htype;
        uint8_t hlen;
        uint8_t hops;
        uint32_t xid;
        uint16_t secs;
        uint16_t flags;
        struct ether_addr ciaddr;
        struct ether_addr yiaddr;
        struct ether_addr siaddr;
        struct ether_addr giaddr;
        struct ether_addr mac_client;
        char * server_name;
        char * file;
        uint8_t dhcp_m_type;
        struct ether_addr client_id;
        uint8_t *options;
};

struct pg_dhcp_state {
	struct pg_brick brick;
	struct ether_addr **mac;
	in_addr_t addr_net;
	in_addr_t addr_broad;
	int *check_ip;
};

struct pg_dhcp_config {
	struct network_addr cidr;
};

static struct pg_brick_config *dhcp_config_new(const char *name, 
                                               const char *cidr)
{
	char cidrip[17];
	strcpy(cidrip, cidr);
        struct pg_brick_config *config;
        struct pg_dhcp_config *dhcp_config;
        dhcp_config = g_new0(struct pg_dhcp_config, 1);
	network_addr_t ip_cidr;
	int ret = str_to_netaddr(cidrip, &ip_cidr);
	if(ret == -1)
		printf("error registering\n");
	print_addr(ip_cidr.addr, ip_cidr.pfx);
	dhcp_config->cidr = ip_cidr;
	print_addr(dhcp_config->cidr.addr, dhcp_config->cidr.pfx);
        config = g_new0(struct pg_brick_config, 1);
        config->brick_config = (void *) dhcp_config;
        return pg_brick_config_init(config, name, 1, 1, PG_MONOPOLE);
}

static int dhcp_poll(struct pg_brick *brick, uint16_t *pkts_cnt,
		     struct pg_error **error)
{
	//struct pg_dhcp_state *state;
	//struct rte_mempool *mp = pg_get_mempool();
	//struct rte_mbuf **pkts;
	//struct pg_brick_side *s;
	//uint64_t pkts_mask;
	//int ret;
	//uint16_t i;


	//state = pg_brick_get_state(brick, struct pg_dhcp_state);

	//pkts = g_new0(struct, state->packets_nb);

	return 0;
}

static int dhcp_burst(struct pg_brick *brick, enum pg_side from,
                     uint16_t edge_index, struct rte_mbuf **pkts,
                     uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_dhcp_state *state;
        struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct ether_addr *mac_pkts;
	uint64_t it_mask;
	uint64_t bit;
	struct rte_mbuf *tmp;
	uint16_t ether_type;
	uint16_t pkts_cnt = 1;
	uint16_t i;
	int j;
	
	state = pg_brick_get_state(brick, struct pg_dhcp_state);

	it_mask = pkts_mask;
	for (; it_mask;) {
		j = 0;
		pg_low_bit_iterate_full(it_mask, bit, i);
		tmp = pkts[i];
		struct ether_hdr *ethernet = (struct ether_hdr *)
				  rte_pktmbuf_mtod(tmp, char *);
		ether_type = pg_utils_get_ether_type(tmp);
		/* DHCP only manage IPv4 adressing
		 * Let non-ip packets (like ARP) pass.		
		 */
		if (unlikely(ether_type != PG_BE_ETHER_TYPE_IPv4)) {
			continue;
		}
		if (RTE_ETH_IS_IPV4_HDR(tmp->packet_type)) {
			if (((tmp->packet_type) & RTE_PTYPE_L4_UDP)) {
				if (is_request(tmp)) {
					while(state->check_ip[j] != 0)
                                                j++;
                                	mac_pkts = &ethernet->s_addr;
                                	state->mac[j] = mac_pkts;
					dhcp_poll(brick, &pkts_cnt, errp);
				}
			}
		}
	}
        return  pg_brick_burst(s->edge.link, from,
                               s->edge.pair_index, pkts, pkts_mask, errp);

}

static int dhcp_init(struct pg_brick *brick,
                    struct pg_brick_config *config,
                    struct pg_error **errp)
{
        struct pg_dhcp_state *state = pg_brick_get_state(brick,
                                                        struct pg_dhcp_state);
	struct pg_dhcp_config *dhcp_config;

	state = pg_brick_get_state(brick, struct pg_dhcp_state);

	dhcp_config = (struct pg_dhcp_config *) config->brick_config;
	state->mac = g_malloc0(pg_calcul_range_ip(dhcp_config->cidr) *
			sizeof(struct ether_addr));
	state->addr_net = network(dhcp_config->cidr.addr,
				  dhcp_config->cidr.pfx);
	state->addr_broad = broadcast(dhcp_config->cidr.addr,
				      dhcp_config->cidr.pfx);
	state->check_ip = g_malloc0(pg_calcul_range_ip(dhcp_config->cidr) *
				   sizeof(bool));

	for (int i = 0 ; i < pg_calcul_range_ip(dhcp_config->cidr); i++) {
		state->check_ip[i] = 0;
	}
        /* initialize fast path */
        brick->burst = dhcp_burst;

        return 0;
}

int pg_calcul_range_ip(network_addr_t ipcidr) {
	return 1 << ((32 - (ipcidr.pfx - 1)) - 1);
}

bool is_discover(struct rte_mbuf *pkts) {
	bool result = true;
	/*struct eth_ip_l4 pkt_dest;
	pkt_dest.ethernet = (rte_pktmbuf_mtod(pkts, struct ether_addr *));
	pkt_dest.ipv4 = pg_utils_get_l3(pkts);
	pkt_dest.v4udp = pg_utils_get_l4(pkts);

	struct eth_ip_l4 pkt_src;
	pkt_src.ethernet = (rte_pktmbuf_mtod(pkts, struct ether_addr *) + 1);
	pkt_src.ipv4 = pg_utils_get_l3(pkts) + 1;
	pkt_src.v4udp = pg_utils_get_l4(pkts) + 1;

	if (pkt_src.ipv4 != "0.0.0.0")
		result = false;
	if (pkt_src.v4udp != 68)
		result = false;
	if (pkt_dest.ethernet != "FF:FF:FF:FF:FF:FF")
		result = false;
	if (pkt_dest.ipv4 != "255.255.255.255")
		result = false;
	if (pkt_dest.v4udp != 67)
		result == false;*/
	return result;
}

bool eth_compare(uint8_t eth1[6], uint8_t eth2[6]) {
        bool result = true;
        for(int i = 0; i < 6 ; i++) {
                if (eth1[i] != eth2[i])
                        result = false;
        }
        return result;
}

bool is_request(struct rte_mbuf *pkts) {

        bool result = true;
	uint8_t eth_dest[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t eth_src[6] = {0x00, 0x18, 0xb9, 0x56, 0x2e, 0x73};
	uint32_t ip_dest;
	inet_pton(AF_INET, "255.255.255.255", &ip_dest);
        uint32_t ip_src;
        inet_pton(AF_INET, "0.0.0.0", &ip_src);
	uint16_t udp_dest = 67;
	uint16_t udp_src = 68;
	uint8_t dhcp_mes_type = 3;
	uint32_t hdrs_len = sizeof(struct ether_hdr) + sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr);

	struct ether_hdr *ethernet = (struct ether_hdr *) rte_pktmbuf_mtod(pkts, char *);
	struct ipv4_hdr *ip = (struct ipv4_hdr *) pg_utils_get_l3(pkts);
        struct udp_hdr *udp = (struct udp_hdr *) pg_utils_get_l4(pkts);
	struct dhcp_messages_payload *dhcp_hdr = (struct dhcp_messages_payload *)
						 rte_pktmbuf_mtod_offset(pkts, char *, hdrs_len);

        if (eth_compare(ethernet->d_addr.addr_bytes, eth_dest) == false)
                result = false;

        if (eth_compare(ethernet->s_addr.addr_bytes, eth_src) == false)
                result = false;

        if (ip->dst_addr != ip_dest)
                result = false;

        if (ip->src_addr != ip_src)
                result = false;

        if (rte_be_to_cpu_16(udp->dst_port) != udp_dest)
                result = false;

        if (rte_be_to_cpu_16(udp->src_port) != udp_src)
                result = false;

	if (dhcp_hdr->dhcp_m_type != dhcp_mes_type)
		result = false;

        return result;
}

struct pg_brick *pg_dhcp_new(const char *name,  const char *cidr,
                             struct pg_error **errp)
{
        struct pg_brick_config *config;
        struct pg_brick *ret;

	config = dhcp_config_new(name, cidr);
	ret = pg_brick_new("dhcp", config, errp);

        pg_brick_config_free(config);

        return ret;
}

static struct pg_brick_ops dhcp_ops = {
        .name           = "dhcp",
        .state_size     = sizeof(struct pg_dhcp_state),

        .init           = dhcp_init,

        .unlink         = pg_brick_generic_unlink,
};

pg_brick_register(dhcp, &dhcp_ops);
