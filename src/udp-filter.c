/* Copyright 2020 Outscale SAS
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

#include <rte_config.h>
#include <rte_errno.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

#include <packetgraph/packetgraph.h>
#include <packetgraph/udp-filter.h>

#include "utils/bitmask.h"
#include "utils/ip.h"
#include "brick-int.h"

struct pg_udp_filter_config {
	enum pg_side to;
	struct pg_udp_filter_info *ports;
	int nb_filters;
};

struct pg_udp_filter_state {
	struct pg_brick brick;
	enum pg_side to;
	struct pg_udp_filter_info *filters;
	int nb_filters;
};

struct header {
	struct ether_hdr ethernet;
	struct ipv4_hdr    ip;
	struct udp_hdr   udp;
}  __attribute__((__packed__));

static int udp_filter_burst(struct pg_brick *brick, enum pg_side from,
			    uint16_t edge_index, struct rte_mbuf **pkts,
			    uint64_t pkts_mask, struct pg_error **errp)
{
	int ret;
	struct pg_udp_filter_state *state =
		pg_brick_get_state(brick, struct pg_udp_filter_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct pg_brick_edge *edges = s->edges;
	int nb_filters = state->nb_filters;
	int nb_ports = nb_filters + 1;
	struct pg_udp_filter_info *filters = state->filters;
	/*
	 * So it's a VLA,  but number of filters should be limited
	 * As it's the packatgraph user that configure it,
	 * If correctly configure we should never have stack overflow
	 */
	uint64_t port_mask[nb_ports];

	if (from == state->to)
		return pg_brick_side_forward(s, from, pkts, pkts_mask, errp);

	memset(port_mask, 0, nb_ports * sizeof(*port_mask));

	/* came from filtered side, so no filters aply here */
	PG_FOREACH_BIT(pkts_mask, i) {
		struct rte_mbuf *pkt = pkts[i];
		struct header *hdr = pg_packet_cast_data(pkt, struct header);

		/* true mean default case */
		if (hdr->ethernet.ether_type != PG_BE_ETHER_TYPE_IPv4 ||
		    hdr->ip.next_proto_id != PG_UDP_PROTOCOL_NUMBER) {
			port_mask[0] |= 1LLU << i;
		}

		for (int j = 0; j < nb_filters; ++j) {
			uint32_t filter_flag = filters[j].flag;

			if (filter_flag & PG_USP_FILTER_DST_PORT &&
			    hdr->udp.dst_port == filters[j].udp_dst_port) {
				port_mask[j + 1] |= 1LLU << i;
				goto next;
			}
			if (filter_flag & PG_USP_FILTER_SRC_PORT &&
			    hdr->udp.src_port == filters[j].udp_src_port) {
				port_mask[j + 1] |= 1LLU << i;
				goto next;
			}
		}
		port_mask[0] |= 1LLU << i;
next:;
	}

	for (int i = 0; i < nb_ports; ++i) {
		ret = pg_brick_burst(edges[i].link,
				     from,
				     edges[i].pair_index,
				     pkts, port_mask[i], errp);
		if (unlikely(ret < 0))
			return ret;
	}
	return 0;
}

static int udp_filter_init(struct pg_brick *brick,
			   struct pg_brick_config *config,
			   struct pg_error **errp)
{
	struct pg_udp_filter_state *s =
		pg_brick_get_state(brick, struct pg_udp_filter_state);
	struct pg_udp_filter_config *fc = config->brick_config;
	int nb_f = fc->nb_filters;
	struct pg_udp_filter_info *pts = fc->ports;

	/* initialize fast path */
	brick->burst = udp_filter_burst;
	s->to = fc->to;
	s->filters = g_new(struct pg_udp_filter_info, nb_f);
	/* let's store port as network endian */
	for (int i = 0; i < nb_f; ++i) {
		s->filters[i].flag = pts[i].flag;
		s->filters[i].udp_src_port =
			rte_cpu_to_be_16(pts[i].udp_src_port);
		s->filters[i].udp_dst_port =
			rte_cpu_to_be_16(pts[i].udp_dst_port);
	}
	s->nb_filters = nb_f;
	return 0;
}

static struct pg_brick_config *config_new(const char *name,
					  uint32_t west_max,
					  uint32_t east_max,
					  enum pg_side output)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_udp_filter_config *udp_filter_config =
		g_new0(struct pg_udp_filter_config, 1);

	udp_filter_config->to = output;
	config->brick_config = udp_filter_config;
	return  pg_brick_config_init(config, name, west_max,
				     east_max, PG_MULTIPOLE);
}

struct pg_brick *pg_udp_filter_new(const char *name,
				   struct pg_udp_filter_info ports[],
				   int nb_filters,
				   enum pg_side output,
				   struct pg_error **errp)
{
	if (!ports) {
		*errp = pg_error_new("port is NULL");
		return NULL;
	}

	int nb_east = output == PG_EAST_SIDE ? nb_filters + 1 : 1;
	int nb_west = output == PG_WEST_SIDE ? nb_filters + 1 : 1;
	struct pg_brick_config *config = config_new(name, nb_west,
						    nb_east,
						    output);
	struct pg_udp_filter_config *udp_filter_config = config->brick_config;
	struct pg_brick *ret;

	udp_filter_config->nb_filters = nb_filters;
	udp_filter_config->ports = ports;
	ret = pg_brick_new("udp_filter", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static void udp_filter_destroy(struct pg_brick *brick,
			       struct pg_error **errp)
{
	struct  pg_udp_filter_state *state =
		pg_brick_get_state(brick, struct pg_udp_filter_state);

	g_free(state->filters);
}

static struct pg_brick_ops udp_filter_ops = {
	.name		= "udp_filter",
	.state_size	= sizeof(struct pg_udp_filter_state),

	.init		= udp_filter_init,

	.destroy	= udp_filter_destroy,
	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(udp_filter, &udp_filter_ops);
