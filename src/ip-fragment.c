/* Copyright 2016 Outscale SAS
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

#include <packetgraph/ip-fragment.h>
#include <rte_config.h>
#include <rte_ip.h>
#include <rte_ip_frag.h>
#include <rte_ether.h>
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "packets.h"
#include "brick-int.h"

struct pg_ip_fragment_config {
	enum pg_side output;
	uint32_t mtu_size;
};

struct pg_ip_fragment_state {
	struct pg_brick brick;
	enum pg_side output;
	struct rte_mbuf *pkts_out[64];
	uint32_t mtu_size;
};

struct eth_ipv4_hdr {
	struct ether_hdr eth;
	struct ipv4_hdr ip;
} __attribute__((__packed__));

static struct pg_brick_config *ip_fragment_config_new(const char *name,
						enum pg_side output,
						uint32_t mtu_size)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_ip_fragment_config *ip_fragment_config =
		g_new0(struct pg_ip_fragment_config, 1);

	ip_fragment_config->output = output;
	ip_fragment_config->mtu_size = mtu_size;
	config->brick_config = (void *) ip_fragment_config;
	return pg_brick_config_init(config, name, 1, 1, PG_DIPOLE);
}

static int do_fragmentation(struct pg_ip_fragment_state *state,
			    struct rte_mbuf *pkt, struct pg_brick_edge *edge,
			    enum pg_side from, struct pg_error **errp)
{
	uint64_t mask;
	int32_t nb_frags;
	struct  eth_ipv4_hdr *pkt_buf = rte_pktmbuf_mtod(pkt,
							 struct eth_ipv4_hdr *);
	struct ether_hdr eth = pkt_buf->eth;
	struct rte_mbuf **pkts_out = state->pkts_out;
	int l2_size = pkt->l2_len;
	int ret;

	rte_pktmbuf_adj(pkt, l2_size);
	/* Because ip RCF ask for the fragment size to be a multiple of 8,
	* and rte_ipv4_fragment_packet don't check if fragment size is
	* a multiple of 8, we need to be sure to give a multiple of 8 for the
	* 4rd argument*/
	nb_frags = rte_ipv4_fragment_packet(pkt, pkts_out, 64,
					    state->mtu_size - l2_size -
					    (8 - ((sizeof(struct ipv4_hdr) +
						   l2_size) % 8)),
					    pg_get_mempool(), pg_get_mempool());
	if (unlikely(nb_frags < 0))
		return -1;
	mask = pg_mask_firsts(nb_frags);

	rte_pktmbuf_prepend(pkt, sizeof(struct  ether_hdr));
	PG_FOREACH_BIT(mask, i) {
		memcpy(rte_pktmbuf_prepend(pkts_out[i],
					   sizeof(struct  ether_hdr)),
		       &eth, sizeof(struct  ether_hdr));
		pkts_out[i]->l2_len = l2_size;
	}

	ret = pg_brick_burst(edge->link, from, edge->pair_index,
			     pkts_out, mask, errp);
	pg_packets_free(pkts_out, mask);
	return ret;
}

static int ip_fragment_burst(struct pg_brick *brick, enum pg_side from,
			     uint16_t edge_index, struct rte_mbuf **pkts,
			     uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_ip_fragment_state *state =
		pg_brick_get_state(brick, struct pg_ip_fragment_state);
	enum pg_side to = pg_flip_side(from);
	struct pg_brick_side *s = &brick->sides[to];

	if (from != state->output) {
		PG_FOREACH_BIT(pkts_mask, i) {
			struct  eth_ipv4_hdr *pkt_buf =
				rte_pktmbuf_mtod(pkts[i],
						 struct eth_ipv4_hdr *);
			int dont_fragment = (pkt_buf->ip.fragment_offset) &
				rte_cpu_to_be_16(IPV4_HDR_DF_FLAG);

			if ((!dont_fragment) &&
			    pkts[i]->pkt_len > state->mtu_size) {
				if (likely(do_fragmentation(state, pkts[i],
							    &s->edge, from,
							    errp) >= 0))
					pkts_mask ^= (ONE64 << i);
			}
		}
	}
	if (!pkts_mask)

		return 0;
	return  pg_brick_burst(s->edge.link, from, s->edge.pair_index,
			       pkts, pkts_mask, errp);
}

struct pg_brick *pg_ip_fragment_new(const char *name,
			      enum pg_side output,
			      uint32_t mtu_size,
			      struct pg_error **errp)
{
	struct pg_brick_config *config = ip_fragment_config_new(name, output,
							  mtu_size);
	struct pg_brick *ret = pg_brick_new("ip_fragment", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static int ip_fragment_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **errp)
{
	struct pg_ip_fragment_state *state =
		pg_brick_get_state(brick, struct pg_ip_fragment_state);
	struct pg_ip_fragment_config *ip_fragment_config;

	ip_fragment_config =
		(struct pg_ip_fragment_config *) config->brick_config;
	if (ip_fragment_config->mtu_size % 8) {
		*errp = pg_error_new("mtu must be a multiplier of 8");
		return -1;
	}

	brick->burst = ip_fragment_burst;
	state->output = ip_fragment_config->output;
	state->mtu_size = ip_fragment_config->mtu_size;
	return 0;
}


static struct pg_brick_ops ip_fragment_ops = {
	.name		= "ip_fragment",
	.state_size	= sizeof(struct pg_ip_fragment_state),

	.init		= ip_fragment_init,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(ip_fragment, &ip_fragment_ops);
