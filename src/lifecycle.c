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

#include <rte_config.h>
#include <rte_eal.h>
#include <rte_vhost.h>
#include <packetgraph/packetgraph.h>
#include "brick-int.h"
#include "utils/mempool.h"
#include "utils/ip.h"
#include "dpdk_symbols.h"

uint16_t pg_base_cksum_ipv4;

int pg_start(int argc, char **argv)
{
	uint32_t flags = 0;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--no-huge"))
			flags |= MEMPOOL_F_NO_PHYS_CONTIG;
	}
	mp_hdlr_init_ops_stack();
	int ret = rte_eal_init(argc, argv);

	pg_alloc_mempool(flags);
	return ret;
}

char **pg_glob_dpdk_argv;
int pg_start_str(const char *dpdk_args)
{
	int dpdk_argc;
	GError *err = NULL;
	char *tmp = g_strdup_printf("dpdk %s", dpdk_args);
	char *pg_glob_dpdk_argv2[128];

	if (!g_shell_parse_argv(tmp, &dpdk_argc, &pg_glob_dpdk_argv, &err))
		return -1;
	if (dpdk_argc > 127)
		return -1;
	memcpy(pg_glob_dpdk_argv2, pg_glob_dpdk_argv,
	       sizeof(char *) * (dpdk_argc + 1));
	int ret = pg_start(dpdk_argc, pg_glob_dpdk_argv2);

	g_free(tmp);
	return ret;
}

void pg_stop(void)
{
	g_list_free(pg_all_bricks);
	pg_all_bricks = NULL;
	g_strfreev(pg_glob_dpdk_argv);
}
