/* Copyright 2016 Outscale SAS
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

#include "fail.h"
#include "brick-int.h"
#include <rte_config.h>
#include <string.h>

struct pg_fail_state {
	struct pg_brick brick;
	enum pg_side output;
};

static int fail_burst(struct pg_brick *brick, enum pg_side from,
			   uint16_t edge_index, struct rte_mbuf **pkts,
			   uint64_t pkts_mask,
			   struct pg_error **errp)
{
	*errp = pg_error_new("Fail brick : %s", brick->name);
	return -1;
}

static int fail_init(struct pg_brick *brick,
			  struct pg_brick_config *config,
			  struct pg_error **errp)
{
	brick->burst = fail_burst;

	return 0;
}

static void fail_link(struct pg_brick *brick, enum pg_side side, int edge)
{
	struct pg_fail_state *state =
	pg_brick_get_state(brick, struct pg_fail_state);

	state->output = pg_flip_side(side);
}

static enum pg_side fail_get_side(struct pg_brick *brick)
{
	struct pg_fail_state *state =
	pg_brick_get_state(brick, struct pg_fail_state);

	return pg_flip_side(state->output);
}

struct pg_brick *pg_fail_new(const char *name,
				  struct pg_error **errp)
{
	struct pg_brick_config *config;

	config = pg_brick_config_new(name, 1, 1, PG_MONOPOLE);
	struct pg_brick *ret = pg_brick_new("fail", config, errp);

	pg_brick_config_free(config);
	return ret;
}
static struct pg_brick_ops fail_ops = {
	.name           = "fail",
	.state_size     = sizeof(struct pg_fail_state),

	.init           = fail_init,
	.link_notify    = fail_link,
	.get_side       = fail_get_side,
	.unlink         = pg_brick_generic_unlink,
};

pg_brick_register(fail, &fail_ops);
