/* Copyright 2015 Outscale SAS
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <netinet/ether.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <arpa/inet.h>
#include <packetgraph/packetgraph.h>

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)

#define DATA_LEN 1000
#define ether_addr_copy(dst, src) memcpy((dst), (src), ETH_ALEN)

uint64_t is_server;
uint64_t tot_packets;
uint64_t tot_packets_in_s;
uint64_t tot_byte;
uint64_t tot_byte_in_s;

struct packet {
	struct ethhdr eth;
	struct iphdr ip;
	struct udphdr udp;
	char payload[DATA_LEN];
} __attribute__((__packed__));

#define PKT_LEN sizeof(struct packet)

static void SignalHandler(int signum)
{
	printf("signal recive, dperf will now end\n");
	if (is_server)
		printf("total recive %lu packets, Mbits %lu\n",
		       tot_packets, (tot_byte * 8) / 1024 / 1024);
	pg_stop();
	exit(0);
}

static void rx_callback(struct pg_brick *brick,
			pg_packet_t **rx_burst,
			uint16_t rx_burst_len,
			void *private_data)
{
	if (!is_server)
		return;
	tot_packets += rx_burst_len;
	tot_packets_in_s += rx_burst_len;
	for (; rx_burst_len; ++rx_burst, --rx_burst_len) {
		tot_byte += pg_packet_len(*rx_burst);
		tot_byte_in_s += pg_packet_len(*rx_burst);
	}
}
static void tx_callback(struct pg_brick *brick,
			pg_packet_t **tx_burst,
			uint16_t *tx_burst_len,
			void *private_data)
{
	static bool copied;

	/*
	 * in server mode, only send first batch of packets so
	 * switch and stuff learn about us
	 */
	if (is_server && copied) {
		*tx_burst_len = 0;
		return;
	}
	if (!copied) {
		for (int i = 0; i < PG_RXTX_MAX_TX_BURST_LEN; i++) {
			memcpy(pg_packet_data(tx_burst[i]),
			       private_data, PKT_LEN);
			pg_packet_set_len(tx_burst[i], PKT_LEN);
		}
		copied = true;
	}
	*tx_burst_len = PG_RXTX_MAX_TX_BURST_LEN;
}

/* dpdk steals -h and --help */
static void check_help(int argc, char **argv)
{
	bool print = false;

	for (int a = 1; a < argc; a++) {
		if (g_str_equal(argv[a], "-h") ||
		    g_str_equal(argv[a], "--help")) {
			print = true;
			break;
		}
	}
	if (print) {
		printf("%s usage: [EAL_OPTIONS] -- [DPERF_OPTIONS]\n", argv[0]);
		printf("\n");
		printf("DPERF_OPTIONS:\n");
		printf("\n");
		printf("  --smac MAC    packet src mac address\n");
		printf("  --dmac MAC    packet dst mac address\n");
		printf("  --sip IP      packet src ip address\n");
		printf("  --dip IP      packet dst ip address\n");
		printf("  --ether TYPE  overwrite ethertype (default:0x0800\n");
		printf("  --time TIME	%s",
		       "run for TIME seconds then stop dperf\n");
		printf("  --server      use as server, %s",
		       "print how many packets have been send, each second\n");
		printf("  --verbose     print packets summary\n");
		printf("  --help, -h    show this help\n");
		printf("\n");
	}
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int ret;
	int timer = 0;
	time_t current_time = time(NULL);
	time_t end = 0;
	int check_time = 0;
	struct pg_brick *nic, *burster, *print;
	struct pg_graph *graph;
	static struct packet pkt;
	struct ether_addr *tmp_mac;
	bool verbose = false;

	check_help(argc, argv);
	ret = pg_start(argc, argv);
	g_assert(ret != -1);
	if (!pg_nic_port_count()) {
		fprintf(stderr, "no dpdk port, ");
		fprintf(stderr, "try using --vdev=eth_pcap0,iface=eth0\n");
		goto exit;
	}

	nic = pg_nic_new_by_id("port 0", 0, &error);
	CHECK_ERROR(error);
	burster = pg_rxtx_new("tx", &rx_callback, &tx_callback, (void *) &pkt);

	pg_nic_get_mac(nic, (struct ether_addr *) &pkt.eth.h_source);
	tmp_mac = ether_aton("ff:ff:ff:ff:ff:ff");
	ether_addr_copy(&pkt.eth.h_dest, tmp_mac);
	inet_aton("10.150.0.1", (struct in_addr *) &pkt.ip.saddr);
	inet_aton("10.150.0.2", (struct in_addr *) &pkt.ip.daddr);
	pkt.eth.h_proto = htons(ETH_P_IP);
	pkt.ip.ihl = 5;
	pkt.ip.version = 4;
	pkt.ip.ttl = 0xff;
	pkt.ip.protocol = IPPROTO_UDP;
	pkt.ip.tot_len = htons(sizeof(struct iphdr) +
			       sizeof(struct udphdr) +
			       DATA_LEN);
	pkt.udp.source = htons(65000);
	pkt.udp.dest = htons(65001);
	pkt.udp.len = htons(DATA_LEN + sizeof(struct udphdr));

	argc -= ret + 1;
	argv += ret + 1;
	ret = 1;
	while (argc > 0) {
		if (g_str_equal(argv[0], "--sip")) {
			if (argc < 2) {
				fprintf(stderr, "--sip needs argument\n");
				goto exit;
			}
			if (!inet_aton(argv[1],
				      (struct in_addr *) &pkt.ip.saddr)) {
				fprintf(stderr, "parsing for --sip failed\n");
				goto exit;
			}
			--argc;
			++argv;
		} else if (g_str_equal(argv[0], "--dip")) {
			if (argc < 2) {
				fprintf(stderr, "--dip needs argument\n");
				goto exit;
			}
			if (!inet_aton(argv[1],
				       (struct in_addr *) &pkt.ip.daddr)) {
				fprintf(stderr, "parsing for --dip failed\n");
				goto exit;
			}
			--argc;
			++argv;
		} else if (g_str_equal(argv[0], "--dmac")) {
			if (argc < 2) {
				fprintf(stderr, "--dmac needs argument\n");
				goto exit;
			}
			struct ether_addr *dst_mac = ether_aton(argv[1]);

			if (!dst_mac) {
				fprintf(stderr, "parsing for --dmac failed\n");
				goto exit;
			}
			ether_addr_copy(&pkt.eth.h_dest, dst_mac);
			--argc;
			++argv;
		} else if (g_str_equal(argv[0], "--smac")) {
			if (argc < 2) {
				fprintf(stderr, "--smac needs argument\n");
				goto exit;
			}
			struct ether_addr *src_mac = ether_aton(argv[1]);

			if (!src_mac) {
				fprintf(stderr, "parsing for --smac failed\n");
				goto exit;
			}
			ether_addr_copy(&pkt.eth.h_source, src_mac);
			--argc;
			++argv;
		} else if (g_str_equal(argv[0], "--ether")) {
			if (argc < 2) {
				fprintf(stderr, "--ether needs argument\n");
				goto exit;
			}
			unsigned long int ether_tmp =
				strtoul(argv[1], NULL, 16);

			if (ether_tmp == 0 || ether_tmp > 65535) {
				fprintf(stderr, "bad --ether argument\n");
				goto exit;
			}
			pkt.eth.h_proto = htons(ether_tmp);
			--argc;
			++argv;
		} else if (g_str_equal(argv[0], "--time")) {
			if (argc < 2) {
				fprintf(stderr, "--time needs argument\n");
				goto exit;
			}
			timer = 1;
			end = current_time + atoi(argv[1]);
			--argc;
			++argv;
		} else if (g_str_equal(argv[0], "--verbose")) {
			verbose = true;
		} else if (g_str_equal(argv[0], "--server")) {
			is_server = true;
		} else {
			fprintf(stderr, "unkown argument %s\n", argv[0]);
			goto exit;
		}
		--argc;
		++argv;
	}
	pkt.ip.check = pg_packet_ipv4_checksum((uint8_t *) &pkt.ip);
	if (verbose) {
		print = pg_print_new("printer", NULL,
				     PG_PRINT_FLAG_SUMMARY |
				     PG_PRINT_FLAG_TIMESTAMP |
				     PG_PRINT_FLAG_SIZE, NULL, &error);
		CHECK_ERROR(error);
		pg_brick_chained_links(&error, nic, print, burster);
	} else {
		pg_brick_link(nic, burster, &error);
	}
	CHECK_ERROR(error);
	graph = pg_graph_new("dperf", nic, &error);
	CHECK_ERROR(error);
	signal(SIGINT, SignalHandler);
	signal(SIGQUIT, SignalHandler);
	signal(SIGSTOP, SignalHandler);
	signal(SIGTERM, SignalHandler);
	printf("let's burst packets !\n");
	while (42) {
		if (pg_graph_poll(graph, &error) < 0) {
			CHECK_ERROR(error);
			break;
		}
		if ((timer || is_server) && check_time & 15) {
			time_t new_time = time(NULL);

			if (is_server && new_time != current_time) {
				printf("recive %lu packets,Mbits %lu\n",
				       tot_packets_in_s,
				       (tot_byte_in_s * 8) / 1024 / 1024);
				tot_packets_in_s = 0;
				tot_byte_in_s = 0;
			}
			current_time = new_time;
			if (timer && current_time == end)
				break;
		}
		++check_time;
	}
	ret = 0;
	if (is_server)
		printf("total recive %lu packets,Mbits %lu\n",
		       tot_packets, (tot_byte * 8) / 1024 / 1024);
exit:
	pg_stop();
	return ret;
}
