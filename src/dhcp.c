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

#include "brick-int.h"
#include "utlis/network.h"
#include "iprange.c"

struct pg_dhcp_state {
	struct pg_brick brick;
	struct ether_addr *mac;
	struct in_addr_t addr_net;
	struct in_addr_t addr_broad;
	enum pg_side outside;
}

struct pg_dhcp_config {
        enum pg_side outside; 
	struct network_addr *cidr;
};

static struct pg_brick_config *dhcp_config_new(const char *name, 
                                               enum pg_side outside,
                                               const char *cidr)
{
        struct pg_brick_config *config;
        struct pg_dhcp_config *dhcp_config;

        dhcp_config = g_new0(struct pg_dhcp_config, 1);
        dhcp_config->outside = outside;
	network_addr_t ip_cidr;
	int ret = str_to_netaddr(cidr, &ipcidr);
	if (ret < 0)
		return -1;
	dhcp_config->cidr = ip_cidr;
        config = g_new0(struct pg_brick_config, 1);
        config->brick_config = (void *) dhcp_config;
        return pg_brick_config_init(config, name, 1, 1, PG_MONOPOLE);
}

static int dhcp_poll(struct pg_brick *brick, uint16_t *pkts_cnt
		     struct pg_error **error)
{
	struct pg_dhcp_state *state;
	struct rte_mempool *mp = pg_get_mempool();
	struct rte_mbuf **pkts;
	struct pg_brick_side *s;
	uint64_t pkts_mask;
	int ret;
	uint16_t i;


	state = pg_brick_get_state(brick, struct pg_dhcp_state);

	pkts = g_new0(struct, state->packets_nb);

	
}

static int dhcp_burst(struct pg_brick *brick, enum pg_side from,
                     uint16_t edge_index, struct rte_mbuf **pkts,
                     uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_dhcp_state *state;
        struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct ether_addr mac_pkts = pg_util_get_ether_src_addr(pkst);
	int i;
	
	while(dhcp_state.mac[i] != NULL)
		i++;
	if (is_request(*pkts))
		dhcp_state.mac[i] = mac_pkts;
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
	state->outside = dhcp_config->outside;
	state->mac = g_malloc0(pg_calcul_range_ip(state->cidr) *
			sizeof(struct ether_addr));
	state->addr_net = network(dhcp_config->cidr.addr,
					config->cidr.pfx);
	state->addr_broad = broadcast(dhcp_config->cidr,
					config->cidr.pfx);
        g_assert(!state->should_be_zero);

        /* initialize fast path */
        brick->burst = dhcp_burst;

        return 0;
}

int pg_calcul_range_ip(network_addr_t ipcidr) {
	return 1 << (32 - (ipcidr.pfx -1)) - 1;
}

bool is_discover(struct rte_mbuf **pkts) {
	bool result = true;
	eth_ip_l4 pkt_dest;
	pkt_dest->ethernet = pg_util_get_ether_dest_addr(*pkts);
	pkt_dest->ipv4 = pg_utils_get_l3(*pkts);
	pkt_dest->v4udp = pg_utils_get_l4(*pkts);

	eth_ip_l4 pkt_src;
	pkt_src->ethernet = pg_util_get_ether_src_addr(*pkts);
	pkt_src->ipv4 = pg_utils_get_l3(*pkts) + 1;
	pkt_src->v4udp = pg_utils_get_l4(*pkts) + 1;

	if (pkt_src->ipv4 != "0.0.0.0")
		result = false;
	if (pkt_src->v4udp != 68)
		result = false;
	if (pkt_dest->ethernet != "FF:FF:FF:FF:FF:FF")
		result = false;
	if (pkt_dest->ipv4 != "255.255.255.255")
		result = false;
	if (pkt_dest->v4udp != 67)
		result == false;
	return result;
}

bool is_request(struct rte_mbuf **pkts) {
        bool result = true;
        eth_ip_l4 pkt_dest;
        pkt_dest->ethernet = pg_util_get_ether_dest_addr(*pkts);
        pkt_dest->ipv4 = pg_utils_get_l3(*pkts);
        pkt_dest->v4udp = pg_utils_get_l4(*pkts);

        eth_ip_l4 pkt_src;
        pkt_src->ethernet = pg_util_get_ether_src_addr(*pkts);
        pkt_src->ipv4 = pg_utils_get_l3(*pkts) + 1;
        pkt_src->v4udp = pg_utils_get_l4(*pkts) + 1;

        if (pkt_src->ipv4 != "0.0.0.0")
                result = false;
        if (pkt_src->v4udp != 68)
                result = false;
        if (pkt_dest->ethernet != "FF:FF:FF:FF:FF:FF")
                result = false;
        if (pkt_dest->ipv4 != "255.255.255.255")
                result = false;
        if (pkt_dest->v4udp != 67)
                result == false;
	if (!pg_utils_get_request_ip(*pkts))
		result == false;
        return result;
}

struct pg_brick *pg_dhcp_new(const char *name,  const char *ipcidr,
                            enum pg_side outside, struct pg_error **errp)
{
        struct pg_brick_config *config;
        struct pg_brick *ret;

	config = dhcp_config_new(name, ipcidr, outside);
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
