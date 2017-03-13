/* Copyright 2016 Outscale SAS
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
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"

#include <rte_config.h>
#include <rte_ip.h>
#include <rte_ether.h>

#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "collect.h"
#include "brick-int.h"

struct eth_ipv4_hdr {
	struct ether_hdr eth;
	struct ipv4_hdr ip;
} __attribute__((__packed__));

static void test_fragment(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *frag;
	struct pg_brick *col_east;
	struct pg_brick *col_west;
	uint64_t mask = pg_mask_firsts(16);
	struct rte_mbuf **pkts = pg_packets_create(mask);
	struct rte_mbuf **tmp_pkts;
	struct ether_addr eth = {{1, 0, 0, 0, 0 ,0}};
	struct eth_ipv4_hdr *pkt_buf, hdr;

	pg_packets_append_ether(pkts, mask,  &eth, &eth,
				ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, mask, 1, 2,
			       1600 - sizeof(struct eth_ipv4_hdr), 0);

	PG_FOREACH_BIT(mask, i) {
		pkt_buf = rte_pktmbuf_mtod(pkts[i], struct eth_ipv4_hdr *);
		pkt_buf->ip.fragment_offset = 0;
		hdr = *pkt_buf;
	}

	pg_packets_append_blank(pkts, mask,
				1600 - sizeof(struct ipv4_hdr) -
				sizeof(struct ether_hdr));

	frag = pg_ip_fragment_new("headshoot !", PG_EAST_SIDE, 432, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	col_west = pg_collect_new("col_west", &error);
	g_assert(!error);

	pg_brick_chained_links(&error, col_west, frag, col_east);
	g_assert(!error);

	for (uint32_t i = 0; i < 4; ++i) {
		mask = pg_mask_firsts(16);

		pg_brick_burst_to_east(frag, 0, pkts, mask, &error);
		g_assert(!error);
		tmp_pkts = pg_brick_west_burst_get(col_east, &mask, &error);
		g_assert(!error);
		g_assert(pg_mask_count(mask) == 4);
		g_assert(pg_brick_pkts_count_get(col_east, PG_EAST_SIDE) ==
			 16 * 4 * (i + 1));
		PG_FOREACH_BIT(mask, i) {
			pkt_buf = rte_pktmbuf_mtod(tmp_pkts[i],
						   struct eth_ipv4_hdr *);

			g_assert(!memcmp(&pkt_buf->eth, &hdr.eth,
					 sizeof(struct ether_hdr)));
			g_assert(pkt_buf->ip.src_addr == hdr.ip.src_addr);
			g_assert(pkt_buf->ip.dst_addr == hdr.ip.dst_addr);
		}
	}

	pg_brick_burst_to_west(frag, 0, tmp_pkts, mask, &error);
	g_assert(!error);
	tmp_pkts = pg_brick_east_burst_get(col_west, &mask, &error);
	g_assert(!error);
	g_assert(pg_mask_count(mask) == 1);

	pg_brick_destroy(col_east);
	pg_brick_destroy(frag);
	pg_packets_free(pkts, mask);
	g_free(pkts);
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int r;

	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);

	/* initialize packetgraph */
	pg_start(argc, argv, &error);
	g_assert(!error);

	pg_test_add_func("/ip_fragment/fragment", test_fragment);
	r = g_test_run();

	pg_stop();
	return r;
}
