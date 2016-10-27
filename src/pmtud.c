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


#include <packetgraph/pmtud.h>
#include <rte_config.h>
#include <rte_ip.h>
#include <rte_ether.h>
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "packets.h"
#include "brick-int.h"

#define ICMP_PROTOCOL_NUMBER 0x01

struct pg_pmtud_config {
	enum pg_side output;
	uint32_t mtu_size;
};

struct pg_pmtud_state {
	struct pg_brick brick;
	enum pg_side output;
	uint32_t eth_mtu_size;
	uint32_t icmp_mtu_size;
	struct rte_mbuf *icmp;
};

struct eth_ipv4_hdr {
	struct ether_hdr eth;
	struct ipv4_hdr ip;
	/* the begin of the message */
	uint64_t beg_msg;
} __attribute__((__packed__));

struct icmp_hdr {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t unused;
	uint16_t next_hop_mtu;
	struct ipv4_hdr ip;
	uint64_t last_msg;
} __attribute__((__packed__));

struct icmp_full_hdr {
	struct ether_hdr eth;
	struct ipv4_hdr ip;
	struct icmp_hdr icmp;
} __attribute__((__packed__));

static struct pg_brick_config *pmtud_config_new(const char *name,
						enum pg_side output,
						uint32_t mtu_size)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_pmtud_config *pmtud_config =
		g_new0(struct pg_pmtud_config, 1);

	pmtud_config->output = output;
	pmtud_config->mtu_size = mtu_size;
	config->brick_config = (void *) pmtud_config;
	return pg_brick_config_init(config, name, 1, 1, PG_DIPOLE);
}

static inline uint16_t icmp_checksum(struct icmp_hdr *msg)
{
	uint16_t sum = 0;

	sum = rte_raw_cksum(msg, sizeof(struct icmp_hdr));
	return ~sum;
}

static int pmtud_burst(struct pg_brick *brick, enum pg_side from,
		       uint16_t edge_index, struct rte_mbuf **pkts,
		       uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_pmtud_state *state =
		pg_brick_get_state(brick, struct pg_pmtud_state);
	enum pg_side to = pg_flip_side(from);
	struct pg_brick_side *s = &brick->sides[to];
	struct pg_brick_side *s_from = &brick->sides[from];
	uint16_t ipv4_proto_be = rte_cpu_to_be_16(ETHER_TYPE_IPv4);
	uint32_t eth_mtu_size = state->eth_mtu_size;

	if (state->output == from) {
		PG_FOREACH_BIT(pkts_mask, i) {
			struct  eth_ipv4_hdr *pkt_buf =
				rte_pktmbuf_mtod(pkts[i],
						 struct eth_ipv4_hdr *);
			int dont_fragment = (pkt_buf->ip.fragment_offset) &
				rte_cpu_to_be_16(IPV4_HDR_DF_FLAG);
			int is_ipv4 =
				(pkt_buf->eth.ether_type == ipv4_proto_be);


			if (is_ipv4 && dont_fragment &&
			    pkts[i]->pkt_len > eth_mtu_size) {
				struct icmp_full_hdr *icmp_buf =
					rte_pktmbuf_mtod(
						state->icmp,
						struct icmp_full_hdr *);

				pkts_mask ^= (ONE64 << i);
				icmp_buf->eth.s_addr = pkt_buf->eth.d_addr;
				icmp_buf->eth.d_addr = pkt_buf->eth.s_addr;
				icmp_buf->ip.src_addr = pkt_buf->ip.dst_addr;
				icmp_buf->ip.dst_addr = pkt_buf->ip.src_addr;
				icmp_buf->ip.hdr_checksum = 0;
				icmp_buf->ip.hdr_checksum =
					rte_ipv4_cksum(&icmp_buf->ip);
				icmp_buf->icmp.ip = pkt_buf->ip;
				icmp_buf->icmp.last_msg = pkt_buf->beg_msg;
				icmp_buf->icmp.checksum = 0;

				icmp_buf->icmp.checksum =
					icmp_checksum(&icmp_buf->icmp);
				if (pg_brick_burst(s_from->edge.link, to,
						   s_from->edge.pair_index,
						   &state->icmp, 1, errp) < 0) {
					return -1;
				}

			}
		}
	}
	if (unlikely(pkts_mask == 0))
		return 0;
	return pg_brick_burst(s->edge.link, from, s->edge.pair_index,
			      pkts, pkts_mask, errp);
}

static int pmtud_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **errp)
{
	struct pg_pmtud_state *state =
		pg_brick_get_state(brick, struct pg_pmtud_state);
	struct pg_pmtud_config *pmtud_config;
	struct ether_addr eth = { {0} };
	struct icmp_hdr *icmp;

	pmtud_config = (struct pg_pmtud_config *) config->brick_config;

	brick->burst = pmtud_burst;

	state->output = pmtud_config->output;
	/* (from rfc1191)
	 * The value carried in the Next-Hop MTU field is:
	 * The size in octets of the largest datagram that could be
	 * forwarded, along the path of the original datagram, without
	 * being fragmented at this router.  The size includes the IP
	 * header and IP data, and does not include any lower-level
	 * headers.
	 */
	state->eth_mtu_size = pmtud_config->mtu_size;
	state->icmp_mtu_size = pmtud_config->mtu_size -
		sizeof(struct ether_hdr);
	state->icmp = rte_pktmbuf_alloc(pg_get_mempool());
	if (!state->icmp) {
		*errp = pg_error_new("cannot allocate icmp packet");
		return -1;
	}
	pg_packets_append_ether(&state->icmp,
				1,  &eth, &eth,
				ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(&state->icmp, 1, 1, 2,
			       sizeof(struct icmp_hdr) +
			       sizeof(struct ipv4_hdr),
			       ICMP_PROTOCOL_NUMBER);
	icmp = (struct icmp_hdr *)rte_pktmbuf_append(state->icmp,
						     sizeof(struct icmp_hdr));
	icmp->type = 3;
	icmp->code = 4;
	icmp->unused = 0;
	icmp->next_hop_mtu = rte_cpu_to_be_16(state->icmp_mtu_size);
	return 0;
}

struct pg_brick *pg_pmtud_new(const char *name,
			      enum pg_side output,
			      uint32_t mtu_size,
			      struct pg_error **errp)
{
	struct pg_brick_config *config = pmtud_config_new(name, output,
							  mtu_size);
	struct pg_brick *ret = pg_brick_new("pmtud", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static void pmtud_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_pmtud_state *state =
		pg_brick_get_state(brick, struct pg_pmtud_state);

	rte_pktmbuf_free(state->icmp);
	state->icmp = NULL;
}

static struct pg_brick_ops pmtud_ops = {
	.name		= "pmtud",
	.state_size	= sizeof(struct pg_pmtud_state),

	.init		= pmtud_init,
	.destroy	= pmtud_destroy,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(pmtud, &pmtud_ops);
