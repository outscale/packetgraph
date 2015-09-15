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

#include <packetgraph/packetgraph.h>
#include <packetgraph/nic.h>
#include <packetgraph/switch.h>
#include <packetgraph/print.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/utils/mac.h>

#define CHECK_ERROR(error) do {			\
		if (error)			\
			pg_error_print(error);	\
		g_assert(!error);		\
	} while (0)


#define PG_BM_ADD(name, brick)			\
	(name = g_list_append(name, brick))

static inline void pg_bm_chained_add_int(GList *name, ...)
{
	va_list ap;
	struct pg_brick *cur;
	
	va_start(ap, name);
	while ((cur = va_arg(ap, struct pg_brick *)) != NULL)
		PG_BM_ADD(name, cur);
	va_end(ap);
}

#define PG_BM_CHAINED_ADD(name, args...)		\
	(pg_bm_chained_add_int(name, args, NULL))


static void pg_brick_destroy_wraper(void *arg)
{
	pg_brick_destroy(arg);
}

#define PG_BM_DESTROY(name) do {					\
		g_list_free_full(name, pg_brick_destroy_wraper);	\
	} while (0)

#define PG_BM_GET_NEXT(name, brick) do {				\
		if (g_list_next(name))					\
			brick = ((name = g_list_next(name))->data);	\
		else							\
			brick = ((name = g_list_first(name))->data);	\
	} while (0)

static int start_loop(int verbose)
{
 	struct pg_error *error = NULL;
	struct pg_brick *nic_tmp, *switch_east, *print_tmp;
	uint16_t port_count = rte_eth_dev_count();
	GList *nic_manager = NULL;
	GList *manager = NULL;

	g_assert(port_count > 1);

	/*
	 * Here is an ascii graph of the links:
	 *
	 * [NIC-X] - [PRINT-X]   --\
	 *		            \
	 * [NIC-X+1] - [PRINT-X+1]   } -- [SWITCH]
	 *		            /
	 * [NIC-X+2] - [PRINT-X+2] /
	 * ....
	 */
	switch_east = pg_switch_new("switch", 20, 20, &error);
	CHECK_ERROR(error);
	PG_BM_ADD(manager, switch_east);

	for (int i = 0; i < port_count; ++i) {
		nic_tmp = pg_nic_new_by_id("nic", 1, 1, WEST_SIDE, i, &error);
		CHECK_ERROR(error);
		print_tmp = pg_print_new("print", 1, 1, NULL,
					 PG_PRINT_FLAG_MAX, NULL,
					 &error);
		CHECK_ERROR(error);
		if (!verbose)
			pg_brick_chained_links(&error, nic_tmp, switch_east);
		else
			pg_brick_chained_links(&error, nic_tmp,
					       print_tmp, switch_east);
		CHECK_ERROR(error);
		PG_BM_ADD(nic_manager, nic_tmp);
		PG_BM_ADD(manager, print_tmp);
	}
	while (1) {
		uint64_t tot_send_pkts = 0;
		for (int i = 0; i < 100000; ++i) {
			uint16_t nb_send_pkts;

			PG_BM_GET_NEXT(nic_manager, nic_tmp);
			pg_brick_poll(nic_tmp, &nb_send_pkts, &error);
			tot_send_pkts += nb_send_pkts;
			CHECK_ERROR(error);
			usleep(1);
		}
		printf("poll pkts: %lu\n", tot_send_pkts);
	}
	nic_manager = g_list_first(nic_manager);
	PG_BM_DESTROY(manager);
	PG_BM_DESTROY(nic_manager);
	return 0;
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	int ret;
	int verbose = 0;
	
	ret = pg_start(argc, argv, &error);
	g_assert(ret != -1);
	CHECK_ERROR(error);
	argc -= ret;
	argv += ret;
	verbose = (argc > 1 && g_str_equal(argv[1], "-verbose"));
	/* accounting program name */
	ret = start_loop(verbose);
	pg_stop();
	return ret;
}
