/* Copyright 2016 Outscale SAS
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


#include <packetgraph/pmtud.h>
#include "utils/bitmask.h"
#include "brick-int.h"

struct pg_pmtud_config {
	enum pg_side output;
	uint32_t mtu_size;
};

struct pg_pmtud_state {
	struct pg_brick brick;
	enum pg_side output;
	uint32_t mtu_size;
};

static struct pg_brick_config *pmtud_config_new(const char *name,
						enum pg_side output,
						uint32_t mtu_size)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_pmtud_config *pmtud_config =
		g_new0(struct pg_pmtud_config, 1);

	pmtud_config->output = output;
	pmtud_config->mtu_size = mtu_size;
	config->brick_config = (void *) pmtud_config;
	return pg_brick_config_init(config, name, 1, 1, PG_DIPOLE);
}

static int pmtud_burst(struct pg_brick *brick, enum pg_side from,
		       uint16_t edge_index, struct rte_mbuf **pkts,
		       uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_pmtud_state *state =
		pg_brick_get_state(brick, struct pg_pmtud_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	uint32_t mtu_size = state->mtu_size;

	if (state->output == from) {
		PG_FOREACH_BIT(pkts_mask, i) {
			if (pkts[i]->pkt_len > mtu_size) {
				pkts_mask ^= (ONE64 << i);
				/* send packet in the other way */
			}
		}
	}
	return pg_brick_burst(s->edge.link, from, s->edge.pair_index,
			      pkts, pkts_mask, errp);
}

static int pmtud_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **errp)
{
	struct pg_pmtud_state *state;
	struct pg_pmtud_config *pmtud_config;

	state = pg_brick_get_state(brick, struct pg_pmtud_state);

	pmtud_config = (struct pg_pmtud_config *) config->brick_config;

	brick->burst = pmtud_burst;

	state->output = pmtud_config->output;
	state->mtu_size = pmtud_config->mtu_size;

	return 0;
}

struct pg_brick *pg_pmtud_new(const char *name,
			      enum pg_side output,
			      uint32_t mtu_size,
			      struct pg_error **errp)
{
	struct pg_brick_config *config = pmtud_config_new(name, output,
							  mtu_size);
	struct pg_brick *ret = pg_brick_new("pmtud", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static struct pg_brick_ops pmtud_ops = {
	.name		= "pmtud",
	.state_size	= sizeof(struct pg_pmtud_state),

	.init		= pmtud_init,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(pmtud, &pmtud_ops);
