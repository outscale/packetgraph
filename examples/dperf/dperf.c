/* Copyright 2015 Outscale SAS
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

#include <unistd.h>
#include <signal.h>
#include <glib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include <rte_config.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ip.h>

#include <packetgraph/packetgraph.h>
#include <packetgraph/nic.h>
#include <packetgraph/vhost.h>
#include <packetgraph/print.h>
#include <packetgraph/brick.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/utils/mac.h>
#include <packetgraph/packets.h>

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)

static int start_loop(int32_t ip_src, int32_t ip_dst,
		      struct ether_addr src_mac,
		      struct ether_addr dst_mac,
		      int mtu)
{
	int ret = -1;
	uint64_t mask = ~0LL;
	struct rte_mbuf **pkts = pg_packets_create(mask);
	struct pg_error *error = NULL;
	struct pg_brick *nic = pg_nic_new_by_id("nic", 1, 1,
						WEST_SIDE, 0, &error);
	CHECK_ERROR(error);
	char	pkt[1500];

	memset(pkt, ~0, 1500);
	pkt[1499] = '\0';

	pg_packets_append_ether(pkts, mask, &src_mac,
				&dst_mac, ETHER_TYPE_IPv4);
	pg_packets_append_ipv4(pkts, mask, ip_src, ip_dst, mtu, 253);
	pkt[mtu - sizeof(struct ipv4_hdr) - sizeof(struct ether_hdr)] = '\0';
	pg_packets_append_str(pkts, mask, pkt);

	while (1) {
		pg_brick_burst_to_west(nic, 0, pkts, 64, mask, &error);
		CHECK_ERROR(error);
		usleep(1);
	}
	return ret;
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int ret = -1;

	struct ether_addr src_mac;
	struct ether_addr dst_mac;
	int32_t src_ip = 0;
	int32_t dst_ip = 0;
	int nbVal = 0;

	ret = pg_start(argc, argv, &error);
	g_assert(ret != -1);
	CHECK_ERROR(error);
	argc -= ret;
	argv += ret;
	while (argc > 1) {
		if (g_str_equal(argv[1], "-sip")) {
			if (argc < 2)
				goto exit;
			src_ip = inet_addr(argv[2]);
			if (src_ip == -1)
				goto exit;
			--argc;
			++argv;
			++nbVal;
		} else if (g_str_equal(argv[1], "-dip")) {
			if (argc < 2)
				goto exit;
			dst_ip = inet_addr(argv[2]);
			if (dst_ip == -1)
				goto exit;
			--argc;
			++argv;
			++nbVal;
		} else if (g_str_equal(argv[1], "-dmac")) {
			if (argc < 2)
				goto exit;
			pg_scan_ether_addr(&dst_mac, argv[2]);
			if (!is_valid_assigned_ether_addr(&dst_mac))
				goto exit;
			--argc;
			++argv;
			++nbVal;
		} else if (g_str_equal(argv[1], "-smac")) {
			if (argc < 2)
				goto exit;
			pg_scan_ether_addr(&src_mac, argv[2]);
			if (!is_valid_assigned_ether_addr(&src_mac))
				goto exit;
			--argc;
			++argv;
			++nbVal;
		}
		--argc;
		++argv;
	}

	if (nbVal != 4)
		goto exit;
	/* accounting program name */
	ret = start_loop(src_ip, dst_ip, src_mac, dst_mac, 1500);
exit:
	pg_stop();
	return ret;
}
