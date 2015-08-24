/* Copyright 2014 Outscale SAS
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
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_eth_ring.h>
#include <unistd.h>
#include <packetgraph/common.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/config.h>
#include <packetgraph/utils/errors.h>
#include <packetgraph/brick.h>
#include <packetgraph/collect.h>
#include <packetgraph/nic.h>
#include "tests.h"

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)

#define NB_PKTS 128

uint16_t max_pkts = PG_MAX_PKTS_BURST;

static void test_nic_simple_flow(void)
{
	struct pg_brick *nic_west, *collect_west, *collect_east, *nic_ring;
	uint64_t pkts_mask;
	int i = 0;
	int nb_iteration = 32;
	uint16_t nb_send_pkts;
	uint16_t total_send_pkts = 0;
	uint16_t total_get_pkts = 0;
	struct pg_error *error = NULL;
	struct pg_nic_stats info;

	/* create a chain of a few nop brick with collectors on each sides */
	/*           /-------- [col_east]
	 * [nic_west] ------- [nic_east]
	 * [col_west] -------/
	 */
	nic_west = pg_nic_new("nic", 4, 4, WEST_SIDE, "eth_pcap0", &error);
	CHECK_ERROR(error);
	nic_ring = pg_nic_new_by_id("nic", 4, 4, EAST_SIDE, 1, &error);
	CHECK_ERROR(error);
	collect_east = pg_collect_new("collect 1", 1, 1, &error);
	CHECK_ERROR(error);
	collect_west = pg_collect_new("collect 2", 1, 1, &error);
	CHECK_ERROR(error);
	pg_brick_link(nic_west, nic_ring, &error);
	CHECK_ERROR(error);
	pg_brick_link(nic_west, collect_east, &error);
	CHECK_ERROR(error);
	pg_brick_link(collect_west, nic_ring, &error);
	CHECK_ERROR(error);

	for (i = 0; i < nb_iteration * 6; ++i) {
		/* max pkts is the maximum nbr of packets
		   rte_eth_burst_wrap can send */
		max_pkts = i * 2;
		if (max_pkts > 64)
			max_pkts = 64;
		struct rte_mbuf **result_pkts;
		/*poll packets to east*/
		pg_brick_poll(nic_west, &nb_send_pkts, &error);
		CHECK_ERROR(error);
		pkts_mask = 0;
		/* collect pkts on the east */
		result_pkts = pg_brick_west_burst_get(collect_east,
						      &pkts_mask, &error);
		CHECK_ERROR(error);
		if (nb_send_pkts) {
			g_assert(result_pkts);
			g_assert(pkts_mask);
			total_send_pkts += max_pkts;
		}
		pkts_mask = 0;
		/* check no pkts end here */
		result_pkts = pg_brick_east_burst_get(collect_east,
						      &pkts_mask, &error);
		CHECK_ERROR(error);
		g_assert(!pkts_mask);
		g_assert(!result_pkts);
		pg_brick_reset(collect_east, &error);
		CHECK_ERROR(error);
	}
	pg_nic_get_stats(nic_ring, &info);
	max_pkts = 64;
	g_assert(info.opackets == total_send_pkts);
	for (i = 0; i < nb_iteration; ++i) {
		/* poll packet to the west */
		pg_brick_poll(nic_ring, &nb_send_pkts, &error);
		CHECK_ERROR(error);
		total_get_pkts += nb_send_pkts;
		pg_brick_east_burst_get(collect_west, &pkts_mask, &error);
		g_assert(pkts_mask);
		pkts_mask = 0;
	}
	/* This assert allow us to check nb_send_pkts*/
	g_assert(total_get_pkts == total_send_pkts);
	g_assert(info.opackets == total_send_pkts);
	/* use packets_count in collect_west here to made
	 * another check when merge*/

	/* break the chain */
	pg_brick_destroy(nic_west);
	pg_brick_destroy(collect_east);
	pg_brick_destroy(collect_west);
	pg_brick_destroy(nic_ring);
}

#undef NB_PKTS
#undef CHECK_ERROR

void test_nic(void)
{
	g_test_add_func("/brick/pcap/nic-pcap", test_nic_simple_flow);
}
