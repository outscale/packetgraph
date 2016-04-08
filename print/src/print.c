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
#include <rte_config.h>
#include <rte_ether.h>
#include <packetgraph/brick.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/print.h>
#include "printer.h"

struct pg_print_config {
	FILE *output;
	uint16_t *type_filter;
	int flags;
};

struct pg_print_state {
	struct pg_brick brick;
	FILE *output;
	uint16_t *type_filter;
	int flags;
	struct timeval start_date;
};

static struct pg_brick_config *pg_print_config_new(const char *name,
						   uint32_t west_max,
						   uint32_t east_max,
						   FILE *output,
						   int flags,
						   uint16_t *type_filter)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_print_config *print_config = g_new0(struct pg_print_config,
						      1);

	print_config->output = output;
	print_config->flags = flags;
	print_config->type_filter = type_filter;

	config->brick_config = (void *) print_config;
	return pg_brick_config_init(config, name, west_max, east_max,
		PG_MULTIPOLE);
}

static int should_skip(uint16_t *type_filter, struct ether_hdr *eth)
{
	int i;

	for (i = 0; type_filter != NULL && type_filter[i]; ++i) {
		if (eth->ether_type == type_filter[i])
			return 1;
	}
	return 0;
}

static int print_burst(struct pg_brick *brick, enum pg_side from,
		       uint16_t edge_index, struct rte_mbuf **pkts,
		       uint16_t nb, uint64_t pkts_mask, struct pg_error **errp)
{

	struct pg_print_state *state;

	state = pg_brick_get_state(brick, struct pg_print_state);
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	uint16_t *type_filter = state->type_filter;
	uint64_t it_mask;
	uint64_t bit;
	int i;
	FILE *o = state->output;
	struct timeval cur;
	uint64_t diff = 0;

	if (state->flags & PG_PRINT_FLAG_TIMESTAMP) {
		gettimeofday(&cur, 0);
		diff = (cur.tv_sec * 1000000 + cur.tv_usec) -
			(state->start_date.tv_sec * 1000000 +
			 state->start_date.tv_usec);
	}

	it_mask = pkts_mask;
	for (; it_mask;) {
		struct ether_hdr *eth;
		void *data;
		size_t size;

		pg_low_bit_iterate_full(it_mask, bit, i);

		/*could be separete in a diferente function*/
		data = rte_pktmbuf_mtod(pkts[i], void*);
		size = rte_pktmbuf_data_len(pkts[i]);
		eth = (struct ether_hdr *) data;

		if (should_skip(type_filter, eth))
			continue;

		if (state->flags & PG_PRINT_FLAG_BRICK) {
			if (from == WEST_SIDE)
				fprintf(o, "-->[%s]", brick->name);
			else
				fprintf(o, "[%s]<--", brick->name);
		}

		if (state->flags & PG_PRINT_FLAG_TIMESTAMP)
			fprintf(o, " [time=%"PRIu64"]", diff);

		if (state->flags & PG_PRINT_FLAG_SIZE)
			fprintf(o, " [size=%"PRIu64"]", size);

		if (state->flags & PG_PRINT_FLAG_SUMMARY)
			print_summary(data, size, o);

		if (state->flags & PG_PRINT_FLAG_RAW)
			print_raw(data, size, o);

		fprintf(o, "\n");
	}
	return pg_brick_side_forward(s, from, pkts, nb, pkts_mask, errp);
}

static int print_init(struct pg_brick *brick,
		      struct pg_brick_config *config,
		      struct pg_error **errp)
{
	struct pg_print_state *state;

	state = pg_brick_get_state(brick, struct pg_print_state);
	struct pg_print_config *print_config;

	if (!config->brick_config) {
		*errp = pg_error_new("config->brick_config is NULL");
		return -1;
	}

	print_config = (struct pg_print_config *) config->brick_config;

	brick->burst = print_burst;

	if (print_config->output == NULL)
		state->output = stdout;
	else
		state->output = print_config->output;
	state->flags = print_config->flags;
	gettimeofday(&state->start_date, 0);
	if (!print_config->type_filter) {
		state->type_filter = NULL;
	} else {
		int i;

		for (i = 0; print_config->type_filter[i]; ++i)
			;
		/* Allocate len + 1 for the end */
		state->type_filter = g_new0(uint16_t, i + 1);
		for (i = 0; print_config->type_filter[i]; ++i)
			state->type_filter[i] = print_config->type_filter[i];
	}

	if (pg_error_is_set(errp))
		return -1;

	return 0;
}

struct pg_brick *pg_print_new(const char *name,
			      uint32_t west_max,
			      uint32_t east_max,
			      FILE *output,
			      int flags,
			      uint16_t *type_filter,
			      struct pg_error **errp)
{
	struct pg_brick_config *config;
	struct pg_brick *ret;

	config = pg_print_config_new(name, west_max,
				     east_max, output,
				     flags, type_filter);
	ret = pg_brick_new("print", config, errp);

	pg_brick_config_free(config);
	return ret;
}

void pg_print_set_flags(struct pg_brick *brick, int flags)
{
	struct pg_print_state *state;

	state = pg_brick_get_state(brick, struct pg_print_state);
	state->flags = flags;
}

static void print_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_print_state *state;

	state = pg_brick_get_state(brick, struct pg_print_state);
	g_free(state->type_filter);
}

static struct pg_brick_ops print_ops = {
	.name		= "print",
	.state_size	= sizeof(struct pg_print_state),

	.init		= print_init,
	.destroy	= print_destroy,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(print, &print_ops);
