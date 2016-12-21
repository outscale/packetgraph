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
#include "utils/tests.h"
#include <packetgraph/pmtud.h>
#include <packetgraph/print.h>

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
	pmtud = pg_pmtud_new("pmtud", PG_WEST_SIDE, 430, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	pg_brick_link(pmtud, col_east, &error);
	g_assert(!error);

	pg_brick_burst(pmtud, PG_WEST_SIDE, 0, pkts, pg_mask_firsts(64), &error);
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
	pmtud = pg_pmtud_new("pmtud", PG_WEST_SIDE, 430, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(!error);
	pg_brick_link(pmtud, col_east, &error);
	g_assert(!error);

	pg_brick_burst(pmtud, PG_WEST_SIDE, 0, pkts, pg_mask_firsts(64), &error);
	g_assert(!error);

	pg_brick_west_burst_get(col_east, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pg_mask_count(pkts_mask) == 32);

	pg_brick_destroy(pmtud);
	pg_brick_destroy(col_east);
	pg_packets_free(pkts, pg_mask_firsts(64));
	g_free(pkts);
}

struct icmp_hdr {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t unused;
	uint16_t next_hop_mtu;
	struct ipv4_hdr ip;
	uint64_t last_msg;
};

struct icmp_full_hdr {
	struct ether_hdr eth;
	struct ipv4_hdr ip;
	struct icmp_hdr icmp;
};

static void test_icmp_pmtud(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *pmtud;
	struct pg_brick *col_east;
	struct pg_brick *col_west;
	/* struct pg_brick *print_east; */
	/* struct pg_brick *print_west; */
	/* FILE *east_file = fopen("east_file.pcap", "w+"); */
	/* FILE *west_file = fopen("west_file.pcap", "w+"); */
	struct rte_mbuf **pkts;
	struct rte_mbuf *tmp;
	uint64_t pkts_mask;
	struct ether_addr eth_s = {{2}};
	struct ether_addr eth_d = {{4}};

	pkts = pg_packets_append_ether(pg_packets_create(pg_mask_firsts(64)),
				       pg_mask_firsts(64),  &eth_s, &eth_d,
				       ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, pg_mask_firsts(64), 1, 2, 0, 0);
	

	/* 10 caracter with the \0*/
	pg_packets_append_str(pkts, pg_mask_firsts(64), "siegzeon ");
	pg_packets_append_blank(pkts,
				pg_mask_firsts(32),
				421 - sizeof(struct ipv4_hdr) -
				sizeof(struct ether_hdr));
	pg_packets_append_blank(pkts,
				pg_mask_firsts(64) & ~pg_mask_firsts(32),
				420 - sizeof(struct ipv4_hdr) -
				sizeof(struct ether_hdr));

	/*
	 * [col_west] -- [print_west] -- [pmtud] -- [print_east] -- [col_east]
	 */

	pmtud = pg_pmtud_new("pmtud", PG_WEST_SIDE, 430, &error);
	g_assert(!error);
	col_east = pg_collect_new("col_east", &error);
	g_assert(col_east);
	g_assert(!error);
	col_west = pg_collect_new("col_west", &error);
	g_assert(!error);
	g_assert(col_west);

	/* print_east = pg_print_new("print_east", 1, 1, east_file, */
	/* 			PG_PRINT_FLAG_PCAP,  NULL, &error); */
	/* g_assert(col_east); */
	/* g_assert(!error); */
	/* print_west = pg_print_new("print_west", 1, 1, west_file, */
	/* 			PG_PRINT_FLAG_PCAP, NULL, &error); */
	/* g_assert(!error); */
	/* g_assert(col_west); */


	/* pg_brick_chained_links(&error, col_west, print_west, pmtud, */
	/* 		       print_east, col_east); */
	/* g_assert(!error); */

	pg_brick_chained_links(&error, col_west, pmtud, col_east);
	g_assert(!error);

	pg_brick_burst_to_east(pmtud, 0, pkts, pg_mask_firsts(64), &error);
	g_assert(!error);

	pg_brick_west_burst_get(col_east, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pg_mask_count(pkts_mask) == 32);

	g_assert(pg_brick_pkts_count_get(pmtud, PG_EAST_SIDE) == 64);
	g_assert(pg_brick_pkts_count_get(col_east, PG_EAST_SIDE) == 32);
	g_assert(pg_brick_pkts_count_get(col_west, PG_WEST_SIDE) == 32);
	tmp = pg_brick_east_burst_get(col_west, &pkts_mask, &error)[0];
	g_assert(pkts_mask == 1);
	g_assert(tmp);

	pg_brick_destroy(col_west);
	pg_brick_destroy(pmtud);
	pg_brick_destroy(col_east);
	pg_packets_free(pkts, pg_mask_firsts(64));
	/* fclose(east_file); */
	/* fclose(west_file); */
	g_free(pkts);
	return;
}

static void test_pmtud(void)
{
	/* tests in the same order as the header function declarations */
	pg_test_add_func("/pmtud/sorting/df", test_sorting_pmtud_df);
	pg_test_add_func("/pmtud/sorting/mtusize", test_sorting_pmtud);
	pg_test_add_func("/pmtud/integrity/icmp", test_icmp_pmtud);
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
