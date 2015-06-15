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
#include "common.h"
#include "utils/mempool.h"
#include "utils/config.h"
#include "bricks/brick.h"
#include "bricks/stubs.h"
#include "tests.h"

#define NB_PKTS 128

static void test_nic_simple_flow(void)
{
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	struct brick *nic_west, *collect_west, *collect_east, *nic_ring;
	uint64_t pkts_mask;
	int i = 0;
	int nb_iteration = 32;
	uint16_t nb_send_pkts;
	uint16_t total_send_pkts = 0;
	uint16_t total_get_pkts = 0;
	struct switch_error *error = NULL;
	struct nic_stats info;

	/* create a chain of a few nop brick with collectors on each sides */
	/*           /-------- [col_east]
	 * [nic_west] ------- [nic_east]
	 * [col_west] -------/
	 */
	nic_west = nic_new("nic", 4, 4, WEST_SIDE, "eth_pcap0", &error);
	g_assert(!error);
	nic_ring = nic_new_by_id("nic", 4, 4, EAST_SIDE, 1, &error);
	g_assert(!error);
	collect_east = brick_new("collect", config, &error);
	g_assert(!error);
	collect_west = brick_new("collect", config, &error);
	g_assert(!error);
	brick_link(nic_west, nic_ring, &error);
	g_assert(!error);
	brick_link(nic_west, collect_east, &error);
	g_assert(!error);
	brick_link(collect_west, nic_ring, &error);
	g_assert(!error);

	for (i = 0; i < nb_iteration * 6; ++i) {
		/* max pkts is the maximum nbr of packets
		   rte_eth_burst_wrap can send */
		max_pkts = i * 2;
		if (max_pkts > 64)
			max_pkts = 64;
		struct rte_mbuf **result_pkts;
		/*poll packets to east*/
		brick_poll(nic_west, &nb_send_pkts, &error);
		g_assert(!error);
		pkts_mask = 0;
		/* collect pkts on the east */
		result_pkts = brick_west_burst_get(collect_east,
						   &pkts_mask, &error);
		g_assert(!error);
		if (nb_send_pkts) {
			g_assert(result_pkts);
			g_assert(pkts_mask);
			total_send_pkts += max_pkts;
		}
		pkts_mask = 0;
		/* check no pkts end here */
		result_pkts = brick_east_burst_get(collect_east,
						   &pkts_mask, &error);
		g_assert(!error);
		g_assert(!pkts_mask);
		g_assert(!result_pkts);
		brick_reset(collect_east, &error);
		g_assert(!error);
	}
	nic_get_stats(nic_ring, &info);
	max_pkts = 64;
	g_assert(info.opackets == total_send_pkts);
	for (i = 0; i < nb_iteration; ++i) {
		/* poll packet to the west */
		brick_poll(nic_ring, &nb_send_pkts, &error);
		g_assert(!error);
		total_get_pkts += nb_send_pkts;
		brick_east_burst_get(collect_west, &pkts_mask, &error);
		g_assert(pkts_mask);
		pkts_mask = 0;
	}
	/* This assert allow us to check nb_send_pkts*/
	g_assert(total_get_pkts == total_send_pkts);
	g_assert(info.opackets == total_send_pkts);
	/* use packets_count in collect_west here to made
	 * another check when merge*/

	/* break the chain */
	brick_destroy(nic_west);
	brick_destroy(collect_east);
	brick_destroy(collect_west);
	brick_destroy(nic_ring);

	brick_config_free(config);
}

#undef NB_PKTS

void test_nic(void)
{
	g_test_add_func("/brick/pcap/nic-pcap",
			test_nic_simple_flow);
}
