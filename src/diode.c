/* Copyright 2014 Outscale SAS
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

#include <packetgraph/packetgraph.h>
#include "brick-int.h"

struct pg_diode_config {
	enum pg_side output;
};

struct pg_diode_state {
	struct pg_brick brick;
	enum pg_side output;
};

static struct pg_brick_config *diode_config_new(const char *name,
						enum pg_side output)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_diode_config *diode_config =
		g_new0(struct pg_diode_config, 1);

	diode_config->output = output;
	config->brick_config = (void *) diode_config;
	return pg_brick_config_init(config, name, 1, 1, PG_DIPOLE);
}

static int diode_burst(struct pg_brick *brick, enum pg_side from,
		       uint16_t edge_index, struct rte_mbuf **pkts,
		       uint64_t pkts_mask)
{
	struct pg_diode_state *state;
	struct pg_brick_side *s;

	state = pg_brick_get_state(brick, struct pg_diode_state);
	s = &brick->sides[pg_flip_side(from)];

	if (state->output == from)
		return 0;

	return pg_brick_burst(s->edge.link, from, s->edge.pair_index,
			      pkts, pkts_mask);
}

static int diode_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **errp)
{
	struct pg_diode_state *state;
	struct pg_diode_config *diode_config;

	state = pg_brick_get_state(brick, struct pg_diode_state);

	diode_config = (struct pg_diode_config *) config->brick_config;

	brick->burst = diode_burst;

	state->output = diode_config->output;
	if (pg_error_is_set(errp))
		return -1;

	return 0;
}

struct pg_brick *pg_diode_new(const char *name,
			      enum pg_side output,
			      struct pg_error **errp)
{
	struct pg_brick_config *config = diode_config_new(name, output);
	struct pg_brick *ret = pg_brick_new("diode", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static struct pg_brick_ops diode_ops = {
	.name		= "diode",
	.state_size	= sizeof(struct pg_diode_state),

	.init		= diode_init,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(diode, &diode_ops);
