/* Copyright 2014 Outscale SAS
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

#include <packetgraph/brick.h>
#include <packetgraph/diode.h>

struct pg_diode_config {
	enum pg_side output;
};

struct pg_diode_state {
	struct pg_brick brick;
	enum pg_side output;
};

static struct pg_brick_config *diode_config_new(const char *name,
						uint32_t west_max,
						uint32_t east_max,
						enum pg_side output)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_diode_config *diode_config =
		g_new0(struct pg_diode_config, 1);

	diode_config->output = output;
	config->brick_config = (void *) diode_config;
	return pg_brick_config_init(config, name, west_max,
				    east_max, PG_MULTIPOLE);
}

static int diode_burst(struct pg_brick *brick, enum pg_side from,
		       uint16_t edge_index, struct rte_mbuf **pkts,
		       uint16_t nb, uint64_t pkts_mask,
		       struct pg_error **errp)
{
	struct pg_diode_state *state;
	struct pg_brick_side *s;

	state = pg_brick_get_state(brick, struct pg_diode_state);
	s = &brick->sides[pg_flip_side(from)];

	if (state->output == from)
		return 1;

	return pg_brick_side_forward(s, from, pkts, nb, pkts_mask, errp);
}

static int diode_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **errp)
{
	struct pg_diode_state *state;
	struct pg_diode_config *diode_config;

	state = pg_brick_get_state(brick, struct pg_diode_state);
	if (!config->brick_config) {
		*errp = pg_error_new("config->brick_config is NULL");
		return 0;
	}

	diode_config = (struct pg_diode_config *) config->brick_config;

	brick->burst = diode_burst;

	state->output = diode_config->output;
	if (pg_error_is_set(errp))
		return 0;

	return 1;
}

struct pg_brick *pg_diode_new(const char *name,
			   uint32_t west_max,
			   uint32_t east_max,
			   enum pg_side output,
			   struct pg_error **errp)
{
	struct pg_brick_config *config = diode_config_new(name, west_max,
							  east_max, output);
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

PG_BRICK_REGISTER(diode, &diode_ops);
