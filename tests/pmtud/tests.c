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

#include <glib.h>
#include <packetgraph/packetgraph.h>
#include <packetgraph/pmtud.h>

#include <rte_config.h>
#include <rte_ip.h>
#include <rte_ether.h>

#include "packetsgen.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "brick-int.h"
#include "packets.h"
#include "collect.h"

static void test_sorting_pmtud(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *pmtud;
	struct pg_brick *col_east;
	struct rte_mbuf **pkts;
	uint64_t pkts_mask;
	struct ether_addr eth = {{0}};

	pkts = pg_packets_append_ether(pg_packets_create(pg_mask_firsts(64)),
				       pg_mask_firsts(64),  &eth, &eth,
				       ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, pg_mask_firsts(64), 1, 2, 0, 0);
	

	pg_packets_append_blank(pkts,
				pg_mask_firsts(32),
				431 - sizeof(struct ipv4_hdr) -
				sizeof(struct ether_hdr));
	pg_packets_append_blank(pkts,
				pg_mask_firsts(64) & ~pg_mask_firsts(32),
				430 - sizeof(struct ipv4_hdr) -
				sizeof(struct ether_hdr));
	pmtud = pg_pmtud_new("pmtud", WEST_SIDE, 430, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", 1, 1, &error);
	g_assert(!error);
	pg_brick_link(pmtud, col_east, &error);
	g_assert(!error);

	pg_brick_burst(pmtud, WEST_SIDE, 0, pkts, pg_mask_firsts(64), &error);
	g_assert(!error);

	pg_brick_west_burst_get(col_east, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pg_mask_count(pkts_mask) == 32);

	pg_brick_destroy(pmtud);
	pg_brick_destroy(col_east);
	pg_packets_free(pkts, pg_mask_firsts(64));
	g_free(pkts);
}


static void test_sorting_pmtud_df(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *pmtud;
	struct pg_brick *col_east;
	struct rte_mbuf **pkts;
	uint64_t pkts_mask;
	struct ether_addr eth = {{0}};
	uint64_t buff[4] = {0, 0, 0, 0};

	pkts = pg_packets_append_ether(pg_packets_create(pg_mask_firsts(64)),
				       pg_mask_firsts(64),  &eth, &eth,
				       ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, pg_mask_firsts(32), 1, 2, 0, 0);
	

	/* Initialise ip header to 0, this ensure that DF flag is not set*/
	pg_packets_append_buf(pkts,
			      pg_mask_firsts(64) & ~pg_mask_firsts(32),
			      buff, sizeof(uint64_t) * 4);
	pg_packets_append_blank(pkts, pg_mask_firsts(64), 400);
	pmtud = pg_pmtud_new("pmtud", WEST_SIDE, 430, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", 1, 1, &error);
	g_assert(!error);
	pg_brick_link(pmtud, col_east, &error);
	g_assert(!error);

	pg_brick_burst(pmtud, WEST_SIDE, 0, pkts, pg_mask_firsts(64), &error);
	g_assert(!error);

	pg_brick_west_burst_get(col_east, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pg_mask_count(pkts_mask) == 32);

	pg_brick_destroy(pmtud);
	pg_brick_destroy(col_east);
	pg_packets_free(pkts, pg_mask_firsts(64));
	g_free(pkts);
}

static void test_pmtud(void)
{
	/* tests in the same order as the header function declarations */
	g_test_add_func("/pmtud/sorting/df", test_sorting_pmtud_df);
	g_test_add_func("/pmtud/sorting/mtusize", test_sorting_pmtud);
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

	test_pmtud();
	r = g_test_run();

	pg_stop();
	return r;
}
