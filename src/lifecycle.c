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
#include <rte_virtio_net.h>
#include <packetgraph/packetgraph.h>
#include "brick-int.h"
#include "dpdk_symbols.h"

int pg_start(int argc, char **argv, struct pg_error **errp)
{
	mp_hdlr_init_ops_stack();
	int r = rte_eal_init(argc, argv);

	if (r < 0)
		*errp = pg_error_new("error durring eal initialisation");
	return r;
}

static char **pg_global_dpdk_args;
int pg_start_str(const char *dpdk_args)
{
	GError *err = NULL;
	int dpdk_argc;
	struct pg_error *errp = NULL;
	char *tmp = g_strdup_printf("dpdk %s", dpdk_args);

	if (!g_shell_parse_argv(tmp, &dpdk_argc, &pg_global_dpdk_args, &err)) {
		goto error;
	}
	pg_start(dpdk_argc, pg_global_dpdk_args, &errp);
	if (pg_error_is_set(&errp)) {
		pg_error_free(errp);
		goto error;
	}
	g_free(tmp);
	return 0;
error:
	g_free(tmp);
	return -1;
}

void pg_stop(void)
{
	g_list_free(pg_all_bricks);
	pg_all_bricks = NULL;
	g_strfreev(pg_global_dpdk_args);
}
