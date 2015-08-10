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

#include <packetgraph/packetgraph.h>
#include <packetgraph/nic.h>
#include <packetgraph/vtep.h>
#include <packetgraph/print.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/utils/mac.h>

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)

struct vtep_opts {
	const char *ip;
	const char *mac;
	const char *inner_mac;
	GList *neighbor_macs;
};

enum test_flags {
	PRINT_USAGE = 1,
	MAC = 2,
	IP = 4,
	I_MAC = 8,
	NB_MACS = 16,
	FAIL = 32
};

struct headers {
	struct ether_hdr ethernet; /* define in rte_ether.h */
	char *data;
} __attribute__((__packed__));

static int quit;

static void sig_handler(int signo)
{
	if (signo == SIGINT)
		quit = 1;
}

static void inverse_mac(struct ether_addr *mac)
{
	for (int i = 0; i < 6; ++i) {
		mac->addr_bytes[i] = ~mac->addr_bytes[i];
	}
}

static int start_loop(uint32_t vtep_ip, struct ether_addr *vtep_mac,
		      struct ether_addr *inner_mac,
		      GList *neighbor_macs)
{
 	struct pg_error *error = NULL;
	struct pg_brick *nic_east, *nic_west, *vtep_east, *vtep_west;
	struct pg_brick *print_east, *print_west, *print_middle;

	/*
	 * Here is an ascii graph of the links:
	 * NIC = nic
	 * VT = vtep
	 *
	 * [NIC] - [PRINT] - [VT] -- [PRINT] -- [VT] -- [PRINT] -- [NIC]
	 */
	nic_east = pg_nic_new_by_id("nic-e", 1, 1, EAST_SIDE, 0, &error);
	CHECK_ERROR(error);
	nic_west = pg_nic_new_by_id("nic-w", 1, 1, WEST_SIDE, 1, &error);
	CHECK_ERROR(error);
	vtep_east = pg_vtep_new("vt-e", 1, 1, WEST_SIDE,
			     vtep_ip, *vtep_mac, 1, &error);
	CHECK_ERROR(error);
	inverse_mac(vtep_mac);
	pg_print_mac(vtep_mac); printf("\n");
	vtep_west = pg_vtep_new("vt-w", 1, 1, EAST_SIDE,
			     ~vtep_ip, *vtep_mac, 1, &error);
	CHECK_ERROR(error);
	print_west = pg_print_new("west", 1, 1, NULL, PG_PRINT_FLAG_MAX, NULL,
			       &error);
	CHECK_ERROR(error);
	print_east = pg_print_new("east", 1, 1, NULL, PG_PRINT_FLAG_MAX, NULL,
			       &error);
	CHECK_ERROR(error);	
	print_middle = pg_print_new("middle", 1, 1, NULL, PG_PRINT_FLAG_MAX,
				    NULL, &error);
	CHECK_ERROR(error);

	/* If you want to print transmiting pkts uncomment this and coment 
	 * the bellow pg_brick_chained_links
	 * Attention: this may slow down the transmition */
	/* pg_brick_chained_links(&error, nic_west, print_west, */
	/* 		    vtep_west, print_middle, vtep_east, */
	/* 		    print_east, nic_east); */
	pg_brick_chained_links(&error, nic_west, vtep_west, vtep_east, nic_east);
	CHECK_ERROR(error);
	pg_vtep_add_vni(vtep_west, nic_west, 0, inet_addr("225.0.0.43"), &error);
	CHECK_ERROR(error);
	pg_vtep_add_vni(vtep_east, nic_east, 0, inet_addr("225.0.0.43"), &error);
	CHECK_ERROR(error);
	while (!quit) {
		uint16_t nb_send_pkts;

		g_assert(pg_brick_poll(nic_west, &nb_send_pkts, &error));
		usleep(1);
		g_assert(pg_brick_poll(nic_east, &nb_send_pkts, &error));
		usleep(1);
	}
	pg_brick_destroy(nic_west);
	pg_brick_destroy(print_west);
	pg_brick_destroy(vtep_west);
	pg_brick_destroy(print_middle);
	pg_brick_destroy(vtep_east);
	pg_brick_destroy(print_east);
	pg_brick_destroy(nic_east);

	return 0;
}

static void print_usage(void)
{
	printf("vxlan usage: [EAL options] -- -m MAC -i IP [options]\n"
	       "vxlan options:\n"
	       "              -h print help\n"
	       "              -m choose ethernet addr of the vtep\n"
	       "              -n neighbor inner macs\n"
	       "              -s self inner mac\n"
	       "              -i choose ip of the vtep\n");
	exit(0);
}

static uint64_t parse_args(int argc, char **argv, struct vtep_opts *opt)
{
	int c = 0;
	uint64_t ret = 0;

	while ((c = getopt(argc, argv, "m:i:hn:s:")) != -1) {
		switch (c) {
		case 'm':
			opt->mac = optarg;
			ret |= MAC;
			break;
		case 'i':
			opt->ip = optarg;
			ret |= IP;
			break;
		case 'n':
			printf("add neighbor\n");
			opt->neighbor_macs = g_list_append(opt->neighbor_macs,
							   optarg);
			ret |= NB_MACS;
			break;
		case 's':
			opt->inner_mac = optarg;
			ret |= I_MAC;
			break;
		case 'h':
			ret |= PRINT_USAGE;
			break;
		case '?':
		default:
			ret |= FAIL;
			break;
		}
	}
	if (!(ret & MAC) || !(ret & IP) || !(ret & NB_MACS) || !(ret & I_MAC))
		ret |= FAIL;
	return ret;
}

static void destroy_ether_addr(void *data)
{
	g_free(data);
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int ret;
	uint64_t args_flags;
	struct vtep_opts opt = {NULL, NULL, NULL, NULL};
	int32_t ip;
	struct ether_addr eth_addr;
	struct ether_addr inner_addr;
	GList *neighbor_addrs = NULL;

	ret = pg_start(argc, argv, &error);
	g_assert(ret != -1);
	CHECK_ERROR(error);

	if (signal(SIGINT, sig_handler) == SIG_ERR)
		return -errno;
	
	/* accounting program name */
	argc -= ret;
	argv += ret;
	args_flags = parse_args(argc, argv, &opt);
	if (args_flags & PRINT_USAGE)
		print_usage();

	if (!!(args_flags & FAIL)) {
		dprintf(2, "Invalide arguments, use '-h'\n");
		ret = -EINVAL;
		goto exit;
	}

	if (!pg_scan_ether_addr(&eth_addr, opt.mac) ||
	    !is_valid_assigned_ether_addr(&eth_addr)) {
		char buf[40];

		ether_format_addr(buf, 40, &eth_addr);
		dprintf(2, "%s is an invalide ethernet adress\n"
			"sould be an unicast addr and have format XX:XX:XX:XX:XX:XX\n",
			buf);
		ret = -EINVAL;
		goto exit;
	}

	if (!pg_scan_ether_addr(&inner_addr, opt.inner_mac) ||
	    !is_valid_assigned_ether_addr(&inner_addr)) {
		char buf[40];

		ether_format_addr(buf, 40, &inner_addr);
		dprintf(2, "%s is an invalide ethernet adress\n"
			"sould be an unicast addr and have format XX:XX:XX:XX:XX:XX\n",
			buf);
		ret = -EINVAL;
		goto exit;
	}

	for (GList *lst = opt.neighbor_macs; lst != NULL;
	     lst = lst->next) {
		const char *data = lst->data;
		struct ether_addr *tmp = g_new0(struct ether_addr, 1);

		if (!pg_scan_ether_addr(tmp, data) ||
		    !is_valid_assigned_ether_addr(tmp)) {
			char buf[40];

			ether_format_addr(buf, 40, tmp);
			dprintf(2, "%s is an invalide ethernet adress\n"
				"sould be an unicast addr and have format XX:XX:XX:XX:XX:XX\n",
				buf);
			ret = -EINVAL;
			goto exit;
		}
		neighbor_addrs = g_list_append(neighbor_addrs, tmp);
	}
		
	ip = inet_addr(opt.ip);
	if (ip < 0) {
		dprintf(2, "invalide ip\n"
			"should have format: XXX.XXX.XXX.XXX\n");
		return -EINVAL;
	}

	ret = start_loop(ip, &eth_addr, &inner_addr, neighbor_addrs);
exit:
	g_list_free(opt.neighbor_macs);
	g_list_free_full(neighbor_addrs, destroy_ether_addr);
	pg_stop();
	return ret;
}
