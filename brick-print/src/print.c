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

#include <stdio.h>
#include <glib.h>
#include <time.h>
#include <sys/time.h>
#include <packetgraph/brick.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/print.h>
#include "printer.h"

struct print_config {
	FILE *output;
	int flags;
};

struct print_state {
	struct brick brick;
	FILE *output;
	int flags;
	struct timeval start_date;
};

static struct brick_config *print_config_new(const char *name,
					     uint32_t west_max,
					     uint32_t east_max,
					     FILE *output,
					     int flags)
{
	struct brick_config *config = g_new0(struct brick_config, 1);
	struct print_config *print_config = g_new0(struct print_config, 1);

	print_config->output = output;
	print_config->flags = flags;

	config->brick_config = (void *) print_config;
	return brick_config_init(config, name, west_max, east_max);
}

static int print_burst(struct brick *brick, enum side from, uint16_t edge_index,
		       struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		       struct switch_error **errp)
{

	struct print_state *state = brick_get_state(brick, struct print_state);
	struct brick_side *s = &brick->sides[flip_side(from)];
	uint64_t it_mask;
	uint64_t bit;
	int i;
	void *data;
	size_t size;
	FILE *o = state->output;
	struct timeval cur;
	uint64_t diff;

	if (state->flags | PRINT_FLAG_TIMESTAMP) {
		gettimeofday(&cur, 0);
		diff = (cur.tv_sec * 1000000 + cur.tv_usec) -
			(state->start_date.tv_sec * 1000000 +
			 state->start_date.tv_usec);
	}

	it_mask = pkts_mask;
	for (; it_mask;) {
		low_bit_iterate_full(it_mask, bit, i);
		data = rte_pktmbuf_mtod(pkts[i], void*);
		size = rte_pktmbuf_data_len(pkts[i]);

		if (state->flags | PRINT_FLAG_BRICK) {
			if (from == WEST_SIDE)
				fprintf(o, "-->[%s]", brick->name);
			else
				fprintf(o, "[%s]<--", brick->name);
		}

		if (state->flags | PRINT_FLAG_TIMESTAMP)
			fprintf(o, " [%"PRIu64"]", diff);

		if (state->flags | PRINT_FLAG_SIZE)
			fprintf(o, " [%"PRIu64"]", size);

		if (state->flags | PRINT_FLAG_SUMMARY)
			print_summary(data, size, o);

		if (state->flags | PRINT_FLAG_RAW)
			print_raw(data, size, o);

		fprintf(o, "\n");
	}
	return brick_side_forward(s, from, pkts, nb, pkts_mask, errp);
}

static int print_init(struct brick *brick,
		      struct brick_config *config, struct switch_error **errp)
{
	struct print_state *state = brick_get_state(brick, struct print_state);
	struct print_config *print_config;

	if (!config->brick_config) {
		*errp = error_new("config->brick_config is NULL");
		return 0;
	}

	print_config = (struct print_config *) config->brick_config;

	brick->burst = print_burst;

	if (print_config->output == NULL)
		state->output = stdout;
	else
		state->output = print_config->output;
	state->flags = print_config->flags;
	gettimeofday(&state->start_date, 0);

	if (error_is_set(errp))
		return 0;

	return 1;
}

struct brick *print_new(const char *name,
			uint32_t west_max,
			uint32_t east_max,
			FILE *output,
			int flags,
			struct switch_error **errp)
{
	struct brick_config *config = print_config_new(name, west_max,
						       east_max, output,
						       flags);
	struct brick *ret = brick_new("print", config, errp);

	brick_config_free(config);
	return ret;
}

void print_set_flags(struct brick *brick, int flags)
{
	struct print_state *state = brick_get_state(brick, struct print_state);

	state->flags = flags;
}

static struct brick_ops print_ops = {
	.name		= "print",
	.state_size	= sizeof(struct print_state),

	.init		= print_init,

	.unlink		= brick_generic_unlink,
};

brick_register(print, &print_ops);
