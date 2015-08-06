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

#include <rte_config.h>
#include <rte_eal.h>
#include <librte_vhost/rte_virtio_net.h>
#include <packetgraph/packetgraph.h>
#include <packetgraph/brick-int.h>

int pg_start(int argc, char **argv, struct pg_error **errp)
{
	int r = rte_eal_init(argc, argv);

	if (r < 0)
		*errp = pg_error_new("error durring eal initialisation");
	return r;
}

void pg_stop(void)
{
	g_list_free(pg_all_bricks);
	pg_all_bricks = NULL;
}
