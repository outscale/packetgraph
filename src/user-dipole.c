/* Copyright 2017 Outscale SAS
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
#include <packetgraph/packetgraph.h>
#include "utils/bitmask.h"
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/network.h"

struct pg_user_dipole_config {
	void *private_data;
	pg_user_dipole_callback_t callback;
};

struct pg_user_dipole_state {
	struct pg_brick brick;
	/* packet graph internal side */
	void *private_data;
	pg_user_dipole_callback_t callback;
};

static struct pg_brick_config *
user_dipole_config_new(const char *name,
		       pg_user_dipole_callback_t callback,
		       void *private_data)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_user_dipole_config *user_dipole_config =
		g_new0(struct pg_user_dipole_config, 1);

	user_dipole_config->private_data = private_data;
	user_dipole_config->callback = callback;
	config->brick_config = (void *) user_dipole_config;
	return pg_brick_config_init(config, name, 1, 1, PG_DIPOLE);
}

static int user_dipole_burst(struct pg_brick *brick, enum pg_side from,
			     uint16_t edge_index, struct rte_mbuf **pkts,
			     uint64_t pkts_mask, struct pg_error **error)
{
	struct pg_user_dipole_state *state =
		pg_brick_get_state(brick, struct pg_user_dipole_state);
	enum pg_side output = pg_flip_side(from);
	struct pg_brick_side *s = &brick->sides[output];

	state->callback(brick, output, pkts, &pkts_mask, state->private_data);
	return pg_brick_burst(s->edge.link, from, s->edge.pair_index,
			      pkts, pkts_mask, error);
}

static int user_dipole_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **error)
{
	struct pg_user_dipole_state *state =
		pg_brick_get_state(brick, struct pg_user_dipole_state);
	struct pg_user_dipole_config *user_dipole_config = config->brick_config;

	state->callback = user_dipole_config->callback;
	state->private_data = user_dipole_config->private_data;
	brick->burst = user_dipole_burst;
	return 0;
}

struct pg_brick *pg_user_dipole_new(const char *name,
				    pg_user_dipole_callback_t callback,
				    void *private_data,
				    struct pg_error **errp)
{
	struct pg_brick_config *config = user_dipole_config_new(name, callback,
							 private_data);
	struct pg_brick *ret = pg_brick_new("user_dipole", config, errp);

	pg_brick_config_free(config);
	return ret;
}

void *pg_user_dipole_private_data(struct pg_brick *brick)
{
	return pg_brick_get_state(brick,
				  struct pg_user_dipole_state)->private_data;
}


static struct pg_brick_ops user_dipole_ops = {
	.name		= "user_dipole",
	.state_size	= sizeof(struct pg_user_dipole_state),
	.init		= user_dipole_init,
	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(user_dipole, &user_dipole_ops);
