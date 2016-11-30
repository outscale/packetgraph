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
#include <glib/gstdio.h>
#include <rte_config.h>
#include <rte_ether.h>
#include <rte_memcpy.h>
#include <packetgraph/packetgraph.h>
#include "utils/bench.h"
#include "utils/bitmask.h"

int pg_bench_init(struct pg_bench *bench, const char *title,
		  int argc, char **argv, struct pg_error **error)
{
	char *output_path = NULL;

	memset(bench, 0, sizeof(struct pg_bench));
	g_strlcpy((char *)bench->title, title, PG_UTILS_BENCH_TITLE_MAX_SIZE);

	for (int i = 1; argv && (i < argc); i++) {
		if (!g_strcmp0("-o", argv[i]) && i + 1 < argc) {
			output_path = argv[i + 1];
			i++;
		} else if (!g_strcmp0("-f", argv[i]) && i + 1 < argc) {
			bench->output_format = argv[i + 1];
			i++;
		}
	}

	if (output_path) {
		bench->output = g_fopen(output_path, "a");
		if (bench->output == NULL) {
			*error = pg_error_new(
					"failed openning %s for writing\n",
					output_path);
			return -1;
		}
	} else {
		bench->output = stdout;
	}

	if (bench->output_format &&
	    g_strcmp0("default", bench->output_format) &&
	    g_strcmp0("csv", bench->output_format)) {
		*error = pg_error_new("%s format not supported\n",
				      bench->output_format);
		return -1;
	}
	return 0;
}

/* This function has been copied from glibc examples
 * http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 */
static bool timeval_subtract(struct timeval *result,
			     struct timeval *x,
			     struct timeval *y)
{
	int nsec;

	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 *  tv_usec is certainly positive.
	 */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

static void pg_bench_burst_cb(void *private_data, uint16_t pkts_burst)
{
	*((uint64_t *)private_data) += pkts_burst;
}

int pg_bench_run(struct pg_bench *bench, struct pg_bench_stats *result,
		 struct pg_error **error)
{
	uint64_t it_mask;
	uint64_t i;
	uint16_t cnt;
	uint64_t pkts_burst;
	struct pg_brick_side *side = NULL;
	struct pg_brick *count_brick;
	struct pg_bench bl;
	uint64_t data_received;
	struct timeval duration;

	if (bench == NULL || result == NULL ||
	    bench->pkts == NULL || bench->pkts_nb == 0 ||
	    bench->max_burst_cnt == 0 || bench->pkts_mask == 0) {
		*error = pg_error_new("missing or bad bench parameters");
		return -1;
	}

	/* Link ouput brick to a nop brick to count outcoming packets. */
	if (bench->count_brick == NULL) {
		count_brick = pg_nop_new("nop-bench", error);
		if (*error)
			return -1;
		if (bench->output_side == WEST_SIDE)
			pg_brick_link(count_brick, bench->output_brick, error);
		else
			pg_brick_link(bench->output_brick, count_brick, error);
		if (*error)
			return -1;
	} else {
		count_brick = bench->count_brick;
	}

	/* Set all stats to zero. */
	memset(result, 0, sizeof(struct pg_bench_stats));

	/* Setup callback to get burst count. */
	pkts_burst = 0;
	switch (bench->input_brick->type) {
	case PG_MONOPOLE:
		side = bench->input_brick->sides;
		break;
	case PG_DIPOLE:
	case PG_MULTIPOLE:
		side = &(bench->input_brick->sides
			 [pg_flip_side(bench->input_side)]);
		break;
	default:
		g_assert(0);
		break;
	}
	side->burst_count_cb = pg_bench_burst_cb;
	side->burst_count_private_data = (void *)(&pkts_burst);

	/* Compute average size of packets. */
	it_mask = bench->pkts_mask;
	for (; it_mask;) {
		pg_low_bit_iterate(it_mask, i);
		result->pkts_average_size +=
			rte_pktmbuf_pkt_len(bench->pkts[i]);
	}
	result->pkts_average_size /= bench->pkts_nb;

	/* Let's run ! */
	rte_memcpy(&bl, bench, sizeof(struct pg_bench));
	gettimeofday(&result->date_start, NULL);
	for (i = 0; i < bl.max_burst_cnt; i++) {
		/* Burst packets. */
		if (unlikely(pg_brick_burst(bl.input_brick,
					    bl.input_side,
					    0,
					    bl.pkts,
					    bl.pkts_mask,
					    error) < 0)) {
			return -1;
		}

		/* Poll back packets if needed. */
		if (bl.output_poll)
			pg_brick_poll(bl.output_brick, &cnt, error);
		if (bl.post_burst_op)
			bl.post_burst_op(bench);
	}
	gettimeofday(&result->date_end, NULL);
	rte_memcpy(bench, &bl, sizeof(struct pg_bench));
	result->pkts_sent = bench->max_burst_cnt * bench->pkts_nb;
	result->burst_cnt = bench->max_burst_cnt;
	result->pkts_burst = pkts_burst;
	result->pkts_received = pg_brick_pkts_count_get(
		count_brick,
		bench->output_side);

	if (bench->count_brick == NULL) {
		pg_brick_unlink(count_brick, error);
		if (*error)
			return -1;
	}

	/* We know that this brick burst all packets. */
	if (bench->brick_full_burst)
		result->pkts_burst = result->pkts_sent;

	/* Compute derivated values from result */
	timeval_subtract(&duration, &result->date_end, &result->date_start);
	result->duration_s = (uint16_t) duration.tv_sec +
		duration.tv_usec / 1000000.0;

	if (result->pkts_burst == 0 ||
	    result->pkts_sent == 0 ||
	    result->duration_s <= 0) {
		printf("pkts_burst: %"PRIu64"\n", result->pkts_burst);
		printf("pkts_received: %"PRIu64"\n", result->pkts_received);
		printf("pkts_received: %lf\n", result->duration_s);
		return -1;
	}

	data_received = result->pkts_received * result->pkts_average_size;

	result->received_packet_speed =
		result->pkts_received / 1000000.0 / result->duration_s;
	result->received_data_speed =
		data_received / 1000000.0 / result->duration_s;
	result->kburst_s =
		result->burst_cnt / result->duration_s / 1000.0;
	result->burst_packets =
		result->pkts_burst * 100.0 / (result->pkts_sent * 1.0);
	result->packet_lost_after_burst =
	100 - result->pkts_received * 100.0 / (result->pkts_burst * 1.0);
	result->total_packet_lost =
		100 - result->pkts_received * 100.0 / (result->pkts_sent * 1.0);
	g_strlcpy(result->title, bench->title, PG_UTILS_BENCH_TITLE_MAX_SIZE);

	/* transfert output parameters */
	result->output = bench->output;
	result->output_format = bench->output_format;
	return 0;
}

void pg_bench_print(struct pg_bench_stats *result)
{
	if (result->output_format == NULL ||
	    !g_strcmp0("default", result->output_format))
		pg_bench_print_default(result);
	else if (!g_strcmp0("csv", result->output_format))
		pg_bench_print_csv(result);
}

void pg_bench_print_default(struct pg_bench_stats *r)
{
	FILE *o = r->output;

	if (o == NULL)
		o = stdout;
	fprintf(o, "================= %s =================\n", r->title);
	fprintf(o, "burst_cnt: %"PRIu64"\n", r->burst_cnt);
	fprintf(o, "pkts_sent: %"PRIu64"\n", r->pkts_sent);
	fprintf(o, "pkts_burst: %"PRIu64"\n", r->pkts_burst);
	fprintf(o, "pkts_received: %"PRIu64"\n", r->pkts_received);
	fprintf(o, "pkts_average_size: %"PRIu64"\n", r->pkts_average_size);
	fprintf(o, "test duration: %lfs\n", r->duration_s);
	fprintf(o, "received packet speed: %.2lf MPkts/s\n",
		r->received_packet_speed);
	fprintf(o, "received data speed: %.4lf MB/s\n", r->received_data_speed);
	fprintf(o, "%.2lf Kbursts/s\n", r->kburst_s);
	fprintf(o, "Burst packets: %.2lf%%\n", r->burst_packets);
	fprintf(o, "packet lost (after burst): %.2lf%%\n",
		r->packet_lost_after_burst);
	fprintf(o, "total packet lost: %.2lf%%\n", r->total_packet_lost);
}

void pg_bench_print_csv_header(FILE *o)
{
	if (o == NULL)
		o = stdout;
	fprintf(o, "test name;");
	fprintf(o, "burst count;");
	fprintf(o, "paquets send;");
	fprintf(o, "packets burst;");
	fprintf(o, "packets received;");
	fprintf(o, "packet average size;");
	fprintf(o, "test duration (s);");
	fprintf(o, "received packet speed (MPkts/s);");
	fprintf(o, "received data speed (MB/s);");
	fprintf(o, "Kbursts/s;");
	fprintf(o, "Bursted packets (%%);");
	fprintf(o, "packet lost after burst (%%);");
	fprintf(o, "total packet lost (%%);");
	fprintf(o, "\n");
}

void pg_bench_print_csv(struct pg_bench_stats *r)
{
	FILE *o = r->output;

	if (o == NULL)
		o = stdout;
	pg_bench_print_csv_header(o);
	fprintf(o, "%s;", r->title);
	fprintf(o, "%"PRIu64";", r->burst_cnt);
	fprintf(o, "%"PRIu64";", r->pkts_sent);
	fprintf(o, "%"PRIu64";", r->pkts_burst);
	fprintf(o, "%"PRIu64";", r->pkts_received);
	fprintf(o, "%"PRIu64";", r->pkts_average_size);
	fprintf(o, "%lf;", r->duration_s);
	fprintf(o, "%.2lf;", r->received_packet_speed);
	fprintf(o, "%.4lf;", r->received_data_speed);
	fprintf(o, "%.2lf;", r->kburst_s);
	fprintf(o, "%.2lf;", r->burst_packets);
	fprintf(o, "%.2lf;", r->packet_lost_after_burst);
	fprintf(o, "%.2lf;", r->total_packet_lost);
	fprintf(o, "\n");
}
