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

#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>
#include <rte_config.h>
#include <rte_common.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <packetgraph/packetgraph.h>
#include "utils/tests.h"
#include "packets.h"
#include "brick-int.h"
#include "utils/bench.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/qemu.h"

void test_benchmark_rxtx(int argc, char **argv);

uint16_t max_pkts = PG_MAX_PKTS_BURST;

struct bench_rxtx {
	struct pg_rxtx_packet pkts[PG_RXTX_MAX_TX_BURST_LEN];
	int len;
};

static void myrx(struct pg_brick *brick,
		 const struct pg_rxtx_packet *rx_burst,
		 uint16_t rx_burst_len,
		 void *private_data)
{
	static int init = 0;
	struct bench_rxtx *pd = (struct bench_rxtx *)private_data;

	if (!init) {
		pd->len = rx_burst_len;
		for (int i = 0; i < rx_burst_len; i++) {
			pd->pkts[i].data = rx_burst[i].data;
			pd->pkts[i].len = rx_burst[i].len;
		}
		init = 1;
	}
}

static void mytx(struct pg_brick *brick,
		 struct pg_rxtx_packet *tx_burst,
		 uint16_t *tx_burst_len,
		 void *private_data)
{
	struct bench_rxtx *pd = (struct bench_rxtx *)private_data;
	int len = pd->len;
	static int init;

	*tx_burst_len = len;
	for (int i = 0; i < len; i++) {
		if (!init)
			memcpy(tx_burst[i].data, pd->pkts[i].data,
			       *pd->pkts[i].len);
		*tx_burst[i].len = *pd->pkts[i].len;
	}
	if (!init)
		init = 1;
}

void test_benchmark_rxtx(int argc, char **argv)
{
	struct pg_error *error = NULL;
	struct pg_brick *rxtx_enter;
	struct pg_brick *rxtx_exit;
	struct pg_bench bench;
	struct pg_bench_stats stats;
	struct ether_addr mac1 = {{0x52,0x54,0x00,0x12,0x34,0x11}};
	struct ether_addr mac2 = {{0x52,0x54,0x00,0x12,0x34,0x21}};
	uint32_t len;
	struct bench_rxtx *pd = g_new0(struct bench_rxtx, 1);

	rxtx_enter = pg_rxtx_new("enter", &myrx, NULL, pd);
	rxtx_exit = pg_rxtx_new("exit", NULL, &mytx, pd);

	g_assert(!pg_bench_init(&bench, "rxtx", argc, argv, &error));
	bench.input_brick = rxtx_enter;
	bench.input_side = PG_WEST_SIDE;
	bench.output_brick = rxtx_exit;
	bench.output_side = PG_WEST_SIDE;
	bench.output_poll = true;
	bench.max_burst_cnt = 1000000;
	bench.count_brick = NULL;
	bench.pkts_nb = 64;
	bench.pkts_mask = pg_mask_firsts(64);
	bench.pkts = pg_packets_create(bench.pkts_mask);
	bench.pkts = pg_packets_append_ether(
		bench.pkts,
		bench.pkts_mask,
		&mac1, &mac2,
		ETHER_TYPE_IPv4);
	len = sizeof(struct ipv4_hdr) + sizeof(struct udp_hdr) + 1400;
	pg_packets_append_ipv4(
		bench.pkts,
		bench.pkts_mask,
		0x000000EE, 0x000000CC, len, 17);
	bench.pkts = pg_packets_append_udp(
		bench.pkts,
		bench.pkts_mask,
		1000, 2000, 1400);
	bench.pkts = pg_packets_append_blank(bench.pkts, bench.pkts_mask, 1400);

	g_assert(!pg_bench_run(&bench, &stats, &error));
	pg_bench_print(&stats);

	pg_packets_free(bench.pkts, bench.pkts_mask);
	pg_brick_destroy(rxtx_enter);
	pg_brick_destroy(rxtx_exit);
	g_free(pd);
}

