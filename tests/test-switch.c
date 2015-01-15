/* Copyright 2014 Nodalink EURL

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

#include "switch-config.h"
#include "bricks/brick.h"
#include "utils/config.h"
#include "utils/mempool.h"
#include "tests.h"

#define NB_PKTS          3

#define TEST_PORTS	40
#define SIDE_PORTS	(TEST_PORTS / 2)
#define TEST_PKTS	8000
#define TEST_MOD	(TEST_PKTS - MAX_PKTS_BURST)
#define TEST_PKTS_COUNT (200 * 1000 * 1000)
#define PKTS_BURST	3

inline uint64_t mask_firsts(uint8_t count);

struct rte_mempool *mbuf_pool;

static inline int scan_ether_addr(struct ether_addr *eth_addr, const char *buf)
{
	int ret;

	/* is the input string long enough */
	if (strlen(buf) < 17)
		return 0;

	ret = sscanf(buf, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
		&eth_addr->addr_bytes[0],
		&eth_addr->addr_bytes[1],
		&eth_addr->addr_bytes[2],
		&eth_addr->addr_bytes[3],
		&eth_addr->addr_bytes[4],
		&eth_addr->addr_bytes[5]);

	/* where the 6 bytes parsed ? */
	return ret == 6;
}

static void set_mac_addrs(struct rte_mbuf *mb, const char *src, const char *dst)
{
	struct ether_hdr *eth_hdr;

	eth_hdr = rte_pktmbuf_mtod(mb, struct ether_hdr *);

	scan_ether_addr(&eth_hdr->s_addr, src);
	scan_ether_addr(&eth_hdr->d_addr, dst);
}

static void test_switch_lifecycle(void)
{
	struct switch_error *error = NULL;
	struct brick *brick;
	struct brick_config *config = brick_config_new("switchbrick", 20, 20);

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	brick_decref(brick, &error);
	brick_config_free(config);
	g_assert(!error);
}

static void test_switch_learn(void)
{
	struct switch_error *error = NULL;
	struct brick *brick, *collect1, *collect2, *collect3;
	struct brick_config *config = brick_config_new("switchbrick", 4, 4);
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	collect1 = brick_new("collect", config, &error);
	g_assert(!error);

	collect2 = brick_new("collect", config, &error);
	g_assert(!error);

	collect3 = brick_new("collect", config, &error);
	g_assert(!error);

	brick_west_link(brick, collect1, &error);
	g_assert(!error);
	brick_west_link(brick, collect2, &error);
	g_assert(!error);
	brick_east_link(brick, collect3, &error);
	g_assert(!error);

	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i],
			      "F0:F1:F2:F3:F4:F5", "E0:E1:E2:E3:E4:E5");
	}

	/* Send the packet through the port 0 of  switch it should been
	 * forwarded on all port because of the learning mode.
	 */
	brick_burst_to_east(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	/* Now send responses packets (source and dest mac address swapped) */
	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "E0:E1:E2:E3:E4:E5", "F0:F1:F2:F3:F4:F5");
	}

	/* through port 0 of east side of the switch */
	brick_burst_to_west(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the switch should not mass learn forward */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have ended in collect 1 (port 0 of west side) */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	/* Now the conversation can continue */
	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "F0:F1:F2:F3:F4:F5", "E0:E1:E2:E3:E4:E5");
	}

	brick_burst_to_east(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the switch should not mass learn forward */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have ended in collect 3 (port 0 of east side) */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the chain */
	brick_unlink(collect1, &error);
	g_assert(!error);
	brick_unlink(collect2, &error);
	g_assert(!error);
	brick_unlink(collect3, &error);
	g_assert(!error);

	brick_decref(collect1, &error);
	g_assert(!error);
	brick_decref(collect2, &error);
	g_assert(!error);
	brick_decref(collect3, &error);
	g_assert(!error);
	brick_decref(brick, &error);

	brick_config_free(config);
	g_assert(!error);
}

static void test_switch_switching(void)
{
	struct switch_error *error = NULL;
	struct brick *brick, *collect1, *collect2, *collect3, *collect4;
	struct brick_config *config = brick_config_new("switchbrick", 4, 4);
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	collect1 = brick_new("collect", config, &error);
	g_assert(!error);

	collect2 = brick_new("collect", config, &error);
	g_assert(!error);

	collect3 = brick_new("collect", config, &error);
	g_assert(!error);

	collect4 = brick_new("collect", config, &error);
	g_assert(!error);

	brick_west_link(brick, collect1, &error);
	g_assert(!error);
	brick_west_link(brick, collect2, &error);
	g_assert(!error);
	brick_east_link(brick, collect3, &error);
	g_assert(!error);
	brick_east_link(brick, collect4, &error);
	g_assert(!error);

	/* we will first exercise the learning abilities of the switch by
	 * sending multiple packets bursts with different source addresses
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i],
			      "10:10:10:10:10:10", "AA:AA:AA:AA:AA:AA");
	}
	/* west port 0 == collect 1 */
	brick_burst_to_east(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);
	brick_reset(collect4, &error);
	g_assert(!error);

	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "22:22:22:22:22:22", "AA:AA:AA:AA:AA:AA");
	}
	/* west port 1 == collect 2 */
	brick_burst_to_east(brick, 1, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);
	brick_reset(collect4, &error);
	g_assert(!error);

	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "66:66:66:66:66:66", "AA:AA:AA:AA:AA:AA");
	}

	/* east port 0 == collect 3 */
	brick_burst_to_west(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);
	brick_reset(collect4, &error);
	g_assert(!error);

	set_mac_addrs(pkts[0], "AA:AA:AA:AA:AA:AA",
			"10:10:10:10:10:10");
	set_mac_addrs(pkts[1], "AA:AA:AA:AA:AA:AA",
			"22:22:22:22:22:22");
	set_mac_addrs(pkts[2], "AA:AA:AA:AA:AA:AA",
			"66:66:66:66:66:66");

	/* east port 1 == collect 4 */
	brick_burst_to_west(brick, 1, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == 0x1);
	g_assert(result_pkts[0]);
	g_assert(result_pkts[0]->udata64 == 0);

	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == 0x2);
	g_assert(result_pkts[1]);
	g_assert(result_pkts[1]->udata64 == 1);

	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == 0x4);
	g_assert(result_pkts[2]);
	g_assert(result_pkts[2]->udata64 == 2);

	result_pkts = brick_west_burst_get(collect4, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* potentially badly addressed packets */
	result_pkts = brick_west_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_west_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect4, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	brick_unlink(collect1, &error);
	g_assert(!error);
	brick_unlink(collect2, &error);
	g_assert(!error);
	brick_unlink(collect3, &error);
	g_assert(!error);
	brick_unlink(collect4, &error);
	g_assert(!error);

	brick_decref(collect1, &error);
	g_assert(!error);
	brick_decref(collect2, &error);
	g_assert(!error);
	brick_decref(collect3, &error);
	g_assert(!error);
	brick_decref(collect4, &error);
	g_assert(!error);
	brick_decref(brick, &error);
	g_assert(!error);

	brick_config_free(config);
	g_assert(!error);
}

static void test_switch_unlink(void)
{
	struct switch_error *error = NULL;
	struct brick *brick, *collect1, *collect2, *collect3, *collect4;
	struct brick_config *config = brick_config_new("switchbrick", 4, 4);
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	collect1 = brick_new("collect", config, &error);
	g_assert(!error);

	collect2 = brick_new("collect", config, &error);
	g_assert(!error);

	collect3 = brick_new("collect", config, &error);
	g_assert(!error);

	collect4 = brick_new("collect", config, &error);
	g_assert(!error);

	brick_west_link(brick, collect1, &error);
	g_assert(!error);
	brick_west_link(brick, collect2, &error);
	g_assert(!error);
	brick_east_link(brick, collect3, &error);
	g_assert(!error);
	brick_east_link(brick, collect4, &error);
	g_assert(!error);

	/* we will first exercise the learning abilities of the switch by
	 * a packets bursts with a given source address
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i],
			      "10:10:10:10:10:10", "AA:AA:AA:AA:AA:AA");
	}
	/* west port 1 == collect 2 */
	brick_burst_to_east(brick, 1, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect1 */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* collect3 */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect4 */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);
	brick_reset(collect4, &error);
	g_assert(!error);

	/* mac address "10:10:10:10:10:10" is now associated with west port 1
	 * brick which is collect2.
	 * We will now unlink and relink collect 2 to exercise the switch
	 * forgetting of the mac addresses associated with a given port.
	 */

	brick_unlink(collect2, &error);
	g_assert(!error);
	brick_west_link(brick, collect2, &error);
	g_assert(!error);

	/* now we send a packet burst to "10:10:10:10:10:10"" and make sure
	 * that the switch unlearned it during the unlink by checking that the
	 * burst is forwarded to all non source ports.
	 */

	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "44:44:44:44:44:44", "10:10:10:10:10:10");
	}

	/* east port 1 == collect 4 */
	brick_burst_to_west(brick, 1, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* check the burst has been forwarded to all port excepted the source
	 * one
	 */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	result_pkts = brick_west_burst_get(collect4, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* potentially badly addressed packets */
	result_pkts = brick_west_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_west_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);
	result_pkts = brick_east_burst_get(collect4, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	brick_unlink(collect1, &error);
	g_assert(!error);
	brick_unlink(collect2, &error);
	g_assert(!error);
	brick_unlink(collect3, &error);
	g_assert(!error);
	brick_unlink(collect4, &error);
	g_assert(!error);

	brick_decref(collect1, &error);
	g_assert(!error);
	brick_decref(collect2, &error);
	g_assert(!error);
	brick_decref(collect3, &error);
	g_assert(!error);
	brick_decref(collect4, &error);
	g_assert(!error);
	brick_decref(brick, &error);

	brick_config_free(config);
	g_assert(!error);
}

/* due to the multicast bit the multicast test also cover the broadcast case */
static void test_switch_multicast_destination(void)
{
	struct switch_error *error = NULL;
	struct brick *brick, *collect1, *collect2, *collect3;
	struct brick_config *config = brick_config_new("switchbrick", 4, 4);
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	collect1 = brick_new("collect", config, &error);
	g_assert(!error);

	collect2 = brick_new("collect", config, &error);
	g_assert(!error);

	collect3 = brick_new("collect", config, &error);
	g_assert(!error);

	brick_west_link(brick, collect1, &error);
	g_assert(!error);
	brick_west_link(brick, collect2, &error);
	g_assert(!error);
	brick_east_link(brick, collect3, &error);
	g_assert(!error);

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i],
			      "AA:AA:AA:AA:AA:AA", "11:11:11:11:11:11");
	}

	/* Send the packet through the port 0 of  switch it should been
	 * forwarded on all port because of the learning mode.
	 */
	brick_burst_to_east(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	/* Now send responses packets (source and dest mac address swapped).
	 * The switch should forward to only one port
	 */
	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "11:11:11:11:11:11", "AA:AA:AA:AA:AA:AA");
	}
	/* through port 0 of east side of the switch */
	brick_burst_to_west(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet hould not have been forwarded to collect2 */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* and collect1 */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	/* Now the conversation can continue */
	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "AA:AA:AA:AA:AA:AA", "11:11:11:11:11:11");
	}

	brick_burst_to_east(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the chain */
	brick_unlink(collect1, &error);
	g_assert(!error);
	brick_unlink(collect2, &error);
	g_assert(!error);
	brick_unlink(collect3, &error);
	g_assert(!error);

	brick_decref(collect1, &error);
	g_assert(!error);
	brick_decref(collect2, &error);
	g_assert(!error);
	brick_decref(collect3, &error);
	g_assert(!error);
	brick_decref(brick, &error);

	brick_config_free(config);
	g_assert(!error);
}
/* due to the multicast bit the multicast test also cover the broadcast case */
static void test_switch_multicast_both(void)
{
	struct switch_error *error = NULL;
	struct brick *brick, *collect1, *collect2, *collect3;
	struct brick_config *config = brick_config_new("switchbrick", 4, 4);
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	collect1 = brick_new("collect", config, &error);
	g_assert(!error);

	collect2 = brick_new("collect", config, &error);
	g_assert(!error);

	collect3 = brick_new("collect", config, &error);
	g_assert(!error);

	brick_west_link(brick, collect1, &error);
	g_assert(!error);
	brick_west_link(brick, collect2, &error);
	g_assert(!error);
	brick_east_link(brick, collect3, &error);
	g_assert(!error);

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i],
			      "33:33:33:33:33:33", "11:11:11:11:11:11");
	}

	/* Send the packet through the port 0 of  switch it should been
	 * forwarded on all port because of the learning mode.
	 */
	brick_burst_to_east(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	/* Now send responses packets (source and dest mac address swapped).
	 * The switch should flood
	 */
	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "11:11:11:11:11:11", "33:33:33:33:33:33");
	}
	/* through port 0 of east side of the switch */
	brick_burst_to_west(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect1 */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* reset the collectors bricks to prepare for next burst */
	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	/* Now the conversation can continue */
	for (i = 0; i < NB_PKTS; i++) {
		set_mac_addrs(pkts[i],
			      "33:33:33:33:33:33", "11:11:11:11:11:11");
	}

	brick_burst_to_east(brick, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	/* the packet should have been forwarded to collect2 */
	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	/* and collect3 */
	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(pkts_mask == mask_firsts(NB_PKTS));
	for (i = 0; i < NB_PKTS; i++) {
		g_assert(result_pkts[i]);
		g_assert(result_pkts[i]->udata64 == i);
	}

	brick_reset(collect1, &error);
	g_assert(!error);
	brick_reset(collect2, &error);
	g_assert(!error);
	brick_reset(collect3, &error);
	g_assert(!error);

	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the chain */
	brick_unlink(collect1, &error);
	g_assert(!error);
	brick_unlink(collect2, &error);
	g_assert(!error);
	brick_unlink(collect3, &error);
	g_assert(!error);

	brick_decref(collect1, &error);
	g_assert(!error);
	brick_decref(collect2, &error);
	g_assert(!error);
	brick_decref(collect3, &error);
	g_assert(!error);
	brick_decref(brick, &error);

	brick_config_free(config);
	g_assert(!error);
}

static void test_switch_filtered(void)
{
#define FILTERED_COUNT 16
	struct switch_error *error = NULL;
	struct brick *brick, *collect1, *collect2, *collect3;
	struct brick_config *config = brick_config_new("switchbrick", 4, 4);
	struct rte_mbuf **result_pkts;
	struct rte_mbuf *pkts[MAX_PKTS_BURST];
	uint64_t pkts_mask, i;

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	collect1 = brick_new("collect", config, &error);
	g_assert(!error);

	collect2 = brick_new("collect", config, &error);
	g_assert(!error);

	collect3 = brick_new("collect", config, &error);
	g_assert(!error);

	brick_west_link(brick, collect1, &error);
	g_assert(!error);
	brick_west_link(brick, collect2, &error);
	g_assert(!error);
	brick_east_link(brick, collect3, &error);
	g_assert(!error);

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < FILTERED_COUNT; i++) {
		char *filtered = g_strdup_printf("01:80:C2:00:00:%02X",
						 (uint8_t) i);

		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i], "33:33:33:33:33:33", filtered);
		g_free(filtered);
	}

	/* Send the packet through the port 0 of  switch it should been
	 * forwarded on all port because of the learning mode.
	 */
	brick_burst_to_east(brick, 0, pkts, FILTERED_COUNT,
			    mask_firsts(FILTERED_COUNT), &error);
	g_assert(!error);

	/* the switch should not do source forward */
	result_pkts = brick_east_burst_get(collect1, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	result_pkts = brick_east_burst_get(collect2, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	result_pkts = brick_west_burst_get(collect3, &pkts_mask, &error);
	g_assert(!error);
	g_assert(!pkts_mask);
	g_assert(!result_pkts);

	for (i = 0; i < FILTERED_COUNT; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the chain */
	brick_unlink(collect1, &error);
	g_assert(!error);
	brick_unlink(collect2, &error);
	g_assert(!error);
	brick_unlink(collect3, &error);
	g_assert(!error);

	brick_decref(collect1, &error);
	g_assert(!error);
	brick_decref(collect2, &error);
	g_assert(!error);
	brick_decref(collect3, &error);
	g_assert(!error);
	brick_decref(brick, &error);

	brick_config_free(config);
	g_assert(!error);

#undef FILTERED_COUNT
}

static void test_switch_perf_learn(void)
{
	struct switch_error *error = NULL;
	struct brick *brick, *nops[TEST_PORTS];
	struct brick_config *config =
		brick_config_new("switchbrick",
				 TEST_PORTS / 2, TEST_PORTS / 2);
	struct brick_config *nop_config = brick_config_new("nop", 1, 1);
	struct rte_mbuf *pkts[TEST_PKTS];
	uint64_t begin, delta;
	uint16_t i;
	uint32_t j;

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	for (i = 0; i < TEST_PORTS; i++) {
		nops[i] = brick_new("nop", nop_config, &error);
		g_assert(!error);
	}

	for (i = 0; i < TEST_PORTS / 2; i++) {
		brick_west_link(brick, nops[i], &error);
		g_assert(!error);
	}

	for (i = 0; i < TEST_PORTS / 2; i++) {
		brick_east_link(brick, nops[i + TEST_PORTS / 2], &error);
		g_assert(!error);
	}

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < TEST_PKTS; i++) {
		uint8_t major, minor;

		major = (i >> 8) & 0xFF;
		minor = i & 0xFF;
		char *mac = g_strdup_printf("AA:AA:AA:AA:%02X:%02X",
					    major, minor);

		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i], mac, "AA:AA:AA:AA:AA:AA");
		g_free(mac);
	}

	/* test the learning speed */
	begin = g_get_real_time();
	for (j = 0; j < TEST_PKTS_COUNT; j += PKTS_BURST) {
		struct rte_mbuf **to_burst = &pkts[j % TEST_MOD];

		brick_burst_to_east(brick, 0,
				 to_burst, PKTS_BURST,
				 mask_firsts(PKTS_BURST), &error);
		g_assert(!error);
	}
	delta = g_get_real_time() - begin;

	printf("Switching %"PRIi64" packets per second in learning mode. ",
		(uint64_t) (TEST_PKTS_COUNT * ((1000000 * 1.0) / delta)));

	for (i = 0; i < TEST_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);


	for (i = 0; i < TEST_PORTS / 2; i++) {
		brick_unlink(nops[i + TEST_PORTS / 2], &error);
		g_assert(!error);
	}
	for (i = 0; i < TEST_PORTS / 2; i++) {
		brick_unlink(nops[i], &error);
		g_assert(!error);
	}

	for (i = 0; i < TEST_PORTS; i++) {
		brick_decref(nops[i], &error);
		g_assert(!error);
	}

	brick_decref(brick, &error);
	g_assert(!error);

	brick_config_free(config);
	g_assert(!error);

}

static void test_switch_perf_switch(void)
{
	struct switch_error *error = NULL;
	struct brick *brick, *nops[TEST_PORTS];
	struct brick_config *config =
		brick_config_new("switchbrick",
				 TEST_PORTS / 2, TEST_PORTS / 2);
	struct brick_config *nop_config = brick_config_new("nop", 1, 1);
	struct rte_mbuf *pkts[TEST_PKTS];
	uint64_t begin, delta;
	struct rte_mbuf **to_burst;
	uint16_t i;
	uint32_t j;

	brick = brick_new("switch", config, &error);
	g_assert(brick);
	g_assert(!error);

	for (i = 0; i < TEST_PORTS; i++) {
		nops[i] = brick_new("nop", nop_config, &error);
		g_assert(!error);
	}

	for (i = 0; i < SIDE_PORTS; i++) {
		brick_west_link(brick, nops[i], &error);
		g_assert(!error);
	}

	for (i = 0; i < SIDE_PORTS; i++) {
		brick_east_link(brick, nops[i + TEST_PORTS / 2], &error);
		g_assert(!error);
	}

	/* take care of sending multicast as source address to make sure the
	 * switch don't learn them
	 */
	for (i = 0; i < TEST_PKTS; i++) {
		uint8_t major, minor;

		major = (i >> 8) & 0xFF;
		minor = i & 0xFF;
		char *mac = g_strdup_printf("AA:AA:AA:AA:%02X:%02X",
						 major, minor);

		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		pkts[i]->udata64 = i;
		set_mac_addrs(pkts[i], mac, "AA:AA:AA:AA:AA:AA");
		g_free(mac);
	}

	/* mac the switch learn the sources mac addresses */
	for (j = 0; j < TEST_PKTS; j += 2) {
		to_burst = &pkts[j];
		brick_burst_to_east(brick, j % SIDE_PORTS, to_burst, 1,
				 mask_firsts(1), &error);
		g_assert(!error);

		to_burst = &pkts[j + 1];
		brick_burst_to_west(brick, j % SIDE_PORTS, to_burst, 1,
				 mask_firsts(1), &error);
		g_assert(!error);
	}

	for (i = 0; i < TEST_PKTS; i++) {
		uint8_t major, minor;

		major = (i >> 8) & 0xFF;
		minor = i & 0xFF;
		char *mac = g_strdup_printf("AA:AA:AA:AA:%02X:%02X",
					    major, minor);
		char *reverse = g_strdup_printf("AA:AA:AA:AA:%02X:%02X",
						 minor, major);


		set_mac_addrs(pkts[i], reverse, mac);
		g_free(mac);
		g_free(reverse);
	}

	begin = g_get_real_time();
	for (j = 0; j < TEST_PKTS_COUNT; j += PKTS_BURST *  2) {
		to_burst = &pkts[(j % TEST_MOD)];
		brick_burst_to_east(brick, SIDE_PORTS - (j % SIDE_PORTS) - 1,
				 to_burst, PKTS_BURST,
				 mask_firsts(PKTS_BURST), &error);
		g_assert(!error);
		to_burst = &pkts[(j + PKTS_BURST) % TEST_MOD];
		brick_burst_to_east(brick, SIDE_PORTS - (j % SIDE_PORTS) - 1,
				 to_burst, PKTS_BURST,
				 mask_firsts(PKTS_BURST), &error);
		g_assert(!error);
	}
	delta = g_get_real_time() - begin;

	printf("Switching %"PRIi64" packets per second in switching mode. ",
		(uint64_t) (TEST_PKTS_COUNT * ((1000000 * 1.0) / delta)));

	for (i = 0; i < TEST_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);


	for (i = 0; i < TEST_PORTS / 2; i++) {
		brick_unlink(nops[i + TEST_PORTS / 2], &error);
		g_assert(!error);
	}
	for (i = 0; i < TEST_PORTS / 2; i++) {
		brick_unlink(nops[i], &error);
		g_assert(!error);
	}

	for (i = 0; i < TEST_PORTS; i++) {
		brick_decref(nops[i], &error);
		g_assert(!error);
	}

	brick_decref(brick, &error);
	g_assert(!error);

	brick_config_free(config);
	g_assert(!error);

#undef TEST_PORTS
#undef TEST_PKTS
#undef TEST_PKTS_COUNT
}
void test_switch(void)
{
	mbuf_pool = get_mempool();
	g_test_add_func("/switch/lifecycle", test_switch_lifecycle);
	g_test_add_func("/switch/learn", test_switch_learn);
	g_test_add_func("/switch/switching", test_switch_switching);
	g_test_add_func("/switch/unlink", test_switch_unlink);
	g_test_add_func("/switch/multicast/destination",
			test_switch_multicast_destination);
	g_test_add_func("/switch/multicast/both", test_switch_multicast_both);
	g_test_add_func("/switch/filtered", test_switch_filtered);
	g_test_add_func("/switch/perf/learn", test_switch_perf_learn);
	g_test_add_func("/switch/perf/switch", test_switch_perf_switch);
}
