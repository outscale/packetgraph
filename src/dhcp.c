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
#include "utils/mac.h"

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
	struct ether_addr mac_dhcp;
	in_addr_t addr_net;
	in_addr_t addr_broad;
	int *check_ip;
	int prefix;
};

struct pg_dhcp_config {
	struct network_addr cidr;
	struct ether_addr mac_dhcp;
};

static struct pg_brick_config *dhcp_config_new(const char *name, 
                                               const char *cidr,
					       struct ether_addr mac_dhcp)
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
	dhcp_config->cidr = ip_cidr;
	dhcp_config->mac_dhcp = mac_dhcp;
        config = g_new0(struct pg_brick_config, 1);
        config->brick_config = (void *) dhcp_config;
        return pg_brick_config_init(config, name, 1, 1, PG_MONOPOLE);
}

static int dhcp_burst(struct pg_brick *brick, enum pg_side from,
                     uint16_t edge_index, struct rte_mbuf **pkts,
                     uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_dhcp_state *state;
        struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct ether_addr *mac_pkts;
	uint64_t it_mask;
	struct rte_mbuf **pkt_offer;
	uint64_t bit;
	struct rte_mbuf *tmp;
	uint16_t ether_type;
	uint16_t i;
	int j;
	uint32_t hdrs_len = sizeof(struct ether_hdr) +
                            sizeof(struct ipv4_hdr) +
                            sizeof(struct udp_hdr);
	
	state = pg_brick_get_state(brick, struct pg_dhcp_state);

	it_mask = pkts_mask;
	for (; it_mask;) {
		j = 0;
		pg_low_bit_iterate_full(it_mask, bit, i);
		tmp = pkts[i];
		struct ether_hdr *eth = (struct ether_hdr *)
				  rte_pktmbuf_mtod(tmp, char *);
        	struct dhcp_messages_payload *dhcp_hdr =
                (struct dhcp_messages_payload *)
                rte_pktmbuf_mtod_offset(tmp, char *, hdrs_len);
		ether_type = pg_utils_get_ether_type(tmp);
		/* DHCP only manage IPv4 adressing
		 * Let non-ip packets (like ARP) pass.		
		 */
		if (unlikely(ether_type != PG_BE_ETHER_TYPE_IPv4)) {
			continue;
		}
		if (RTE_ETH_IS_IPV4_HDR(tmp->packet_type)) {
			if (((tmp->packet_type) & RTE_PTYPE_L4_UDP)) {
				if (is_discover(tmp)) {
					while(state->check_ip[j] != 0)
                                                j++;
					in_addr_t ip_offer =
						state->addr_net + j;
					pkt_offer =
					pg_dhcp_packet_create(brick, 2,
						eth->s_addr, ip_offer, 3600,
						state->addr_net,
						netmask(state->prefix));
					return  pg_brick_burst(s->edge.link,
					from, s->edge.pair_index, pkt_offer,
					pkts_mask, errp);
				}
				if (is_request(tmp)) {
					uint32_t index = dhcp_hdr->request_ip -
							 state->addr_net;
					if (!state->check_ip[index])
					{
						pkt_offer =
                                		pg_dhcp_packet_create(brick, 5,
							eth->s_addr,
							dhcp_hdr->request_ip, 3600,
                                                	state->addr_net,
                                                	netmask(state->prefix));
                                        	mac_pkts = &eth->s_addr;
                                        	state->mac[j] = mac_pkts;
                                        	return  pg_brick_burst(s->edge.link,
                                        	from, s->edge.pair_index, pkt_offer,
                                        	pkts_mask, errp);
					}
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
	state->mac_dhcp = dhcp_config->mac_dhcp;
	state->addr_net = network(dhcp_config->cidr.addr,
				  dhcp_config->cidr.pfx);
	state->addr_broad = broadcast(dhcp_config->cidr.addr,
				      dhcp_config->cidr.pfx);
	state->check_ip = g_malloc0(pg_calcul_range_ip(dhcp_config->cidr) *
				   sizeof(bool));
	state->prefix = dhcp_config->cidr.pfx;

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

#define PG_PACKETS_TEST_DHCP(pkts, msg_type)			   \
	bool result = true;					   \
	uint8_t eth_dest[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};\
	uint8_t eth_src[6] = {0x00, 0x18, 0xb9, 0x56, 0x2e, 0x73}; \
	uint32_t ip_dest;					   \
	inet_pton(AF_INET, "255.255.255.255", &ip_dest);	   \
	uint32_t ip_src;					   \
	inet_pton(AF_INET, "0.0.0.0", &ip_src);			   \
	uint16_t udp_dest = 67;					   \
	uint16_t udp_src = 68;					   \
	uint8_t dhcp_mes_type = msg_type;			   \
	uint32_t hdrs_len = sizeof(struct ether_hdr) +		   \
			    sizeof(struct ipv4_hdr) +		   \
			    sizeof(struct udp_hdr);		   \
	struct ether_hdr *ethernet = (struct ether_hdr *)	   \
				    rte_pktmbuf_mtod(pkts, char *);\
	struct ipv4_hdr *ip = (struct ipv4_hdr *)		   \
			      pg_utils_get_l3(pkts);		   \
	struct udp_hdr *udp = (struct udp_hdr *)		   \
			      pg_utils_get_l4(pkts);		   \
	struct dhcp_messages_payload *dhcp_hdr =		   \
		(struct dhcp_messages_payload *)		   \
		rte_pktmbuf_mtod_offset(pkts, char *, hdrs_len);   \
								   \
	if (eth_compare(ethernet->d_addr.addr_bytes, eth_dest) == false)  \
		result = false;						  \
	if (eth_compare(ethernet->s_addr.addr_bytes, eth_src) == false)   \
		result = false;						  \
	if (ip->dst_addr != ip_dest)					  \
		result = false;						  \
	if (ip->src_addr != ip_src)					  \
		result = false;						  \
	if (rte_be_to_cpu_16(udp->dst_port) != udp_dest)		  \
		result = false;						  \
	if (rte_be_to_cpu_16(udp->src_port) != udp_src)			  \
		result = false;						  \
	if (dhcp_hdr->dhcp_m_type != dhcp_mes_type)			  \
		result = false;						  \
									  \
	return result;							  \
	

bool is_discover(struct rte_mbuf *pkts) {
	PG_PACKETS_TEST_DHCP(pkts, 1);
}

bool is_request(struct rte_mbuf *pkts) {
	PG_PACKETS_TEST_DHCP(pkts, 3);
}

bool eth_compare(uint8_t eth1[6], uint8_t eth2[6]) {
        bool result = true;
        for(int i = 0; i < 6 ; i++) {
                if (eth1[i] != eth2[i])
                        result = false;
        }
        return result;
}

struct rte_mbuf **pg_dhcp_packet_create(struct pg_brick *brick,
				       uint8_t msg_type, struct ether_addr mac,
				       uint32_t ip_offer, int bail,
				       uint32_t ip_router, uint32_t mask)
{
	struct rte_mbuf **pkts;
	struct ether_addr eth_dst;
	struct ether_addr eth_src;
	struct dhcp_messages_payload dhcp_hdr;
        char *tmp;
	struct pg_dhcp_state *state;

	state = pg_brick_get_state(brick, struct pg_dhcp_state);
	
	pg_scan_ether_addr(&eth_dst, "FF:FF:FF:FF:FF:FF");
	eth_src = state->mac_dhcp;

	pkts = pg_packets_append_ether(pg_packets_create(pg_mask_firsts(1)),
                                       pg_mask_firsts(1), &eth_src, &eth_dst,
                                       ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, pg_mask_firsts(1), state->addr_net,
			       0xFFFFFFFF, sizeof(struct ipv4_hdr), 17);
	pg_packets_append_udp(pkts, pg_mask_firsts(1), 68, 67,
			      sizeof(struct udp_hdr));

        PG_FOREACH_BIT(pg_mask_firsts(1), j) {
        if (!pkts[j])
                continue;
        dhcp_hdr.mac_client = mac;
        dhcp_hdr.dhcp_m_type = msg_type;
	dhcp_hdr.subnet_mask = mask;
	dhcp_hdr.yiaddr = ip_offer;
	dhcp_hdr.router_ip = ip_router;
	dhcp_hdr.lease_time = bail;
	dhcp_hdr.server_identifier = state->addr_net;
        tmp = rte_pktmbuf_append(pkts[j],
		sizeof(struct dhcp_messages_payload));
        if (!tmp)
                printf("error buffer\n");
        memcpy(tmp, &dhcp_hdr, sizeof(struct dhcp_messages_payload));
        }

	return pkts;
}

struct pg_brick *pg_dhcp_new(const char *name,  const char *cidr,
                             struct ether_addr mac_dhcp,
			     struct pg_error **errp)
{
        struct pg_brick_config *config;
        struct pg_brick *ret;

	config = dhcp_config_new(name, cidr, mac_dhcp);
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
