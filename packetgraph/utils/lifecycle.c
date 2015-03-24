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
#include <rte_virtio_net.h>

#include "utils/lifecycle.h"
#include "bricks/vhost-user-brick.h"

int packetgraph_start(int argc, char **argv,
		      const char *base_dir,
		      struct switch_error **errp)
{
	if (rte_eal_init(argc, argv) < 0) {
		error_new("error durring eal initialisation");
		return 0;
	}
	if (!vhost_start(base_dir, errp))
		return 0;
	return 1;
}

void packetgraph_stop(void)
{
	vhost_stop();
}
