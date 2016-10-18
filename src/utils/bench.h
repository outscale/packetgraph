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

#ifndef _PG_UTILS_BENCH_H
#define _PG_UTILS_BENCH_H

#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include <packetgraph/common.h>
#include "brick-int.h"

#define PG_UTILS_BENCH_TITLE_MAX_SIZE 1000

struct pg_bench_stats {
	/* Burst count. */
	uint64_t burst_cnt;
	/* Total of packets (burst succesiful or not). */
	uint64_t pkts_sent;
	/* Total of packets who really bursted in input brick. */
	uint64_t pkts_burst;
	/* Total of pulled packets. */
	uint64_t pkts_received;
	/* Average packet size. */
	uint64_t pkts_average_size;
	/* Start date of the test. */
	struct timeval date_start;
	/* End date of the test. */
	struct timeval date_end;
	/* test duration in seconds */
	double duration_s;
	/* received packet speed in MPkts */
	double received_packet_speed;
	/* received data speed in MB/s */
	double received_data_speed;
	/* Number of bursts made (Kburst/s) */
	double kburst_s;
	/* Number of successfully burst packets (%) */
	double burst_packets;
	/* Number of lost packets after burst (%) */
	double packet_lost_after_burst;
	/* Total number of lost packets (%) */
	double total_packet_lost;
	/* Test title */
	char title[PG_UTILS_BENCH_TITLE_MAX_SIZE];
	/* Output where to write benchmark, NULL will write to stdout */
	FILE *output;
	/* Output format, can be: NULL or "default" */
	char *output_format;
};

struct pg_bench {
	/* Brick under test which receive packets. */
	struct pg_brick *input_brick;
	/* Side where packets are supposed to come in input_brick. */
	enum pg_side input_side;
	/* Brick under test where packets pop out.
	 * Note: can be the same brick than input.
	 */
	struct pg_brick *output_brick;
	/* Side where packets are supposed to come out from output_brick. */
	enum pg_side output_side;
	/* Set this to true to explicitly poll packets from output brick. */
	bool output_poll;
	/* Burst of packets bench test should continuously send. */
	struct rte_mbuf **pkts;
	uint16_t pkts_nb;
	uint64_t pkts_mask;
	/* Number of burst to make */
	uint64_t max_burst_cnt;
	/* Does this brick bursts all it's packets ? */
	int brick_full_burst;
	/* A nop brick is connected to output_brick in order to count
	 * incomming packets. If you set count_brick to NULL, a nop brick
	 * will be linked during the benchmark. If you set your own brick,
	 * You will be responsible to link it to your output_brick before
	 * launching a benchmark.
	 */
	struct pg_brick *count_brick;
	void (*post_burst_op)(struct pg_bench *);
	/* Test title */
	char title[PG_UTILS_BENCH_TITLE_MAX_SIZE];
	/* Output where to write benchmark, NULL will write to stdout */
	FILE *output;
	/* Output format, can be: NULL or "default" */
	char *output_format;
};

/**
 * Initialize benchmark
 *
 * @param   bench benchmark configuration to init.
 * @param   title benchmark title.
 * @param   argc program's argc (ignored if argv is NULL)
 * @param   argv program's argv (optional, can be NULL)
 * @param   error is set in case of an error
 * @return  0 on success, -1 on error.
 */
int pg_bench_init(struct pg_bench *bench, const char *title,
		  int argc, char **argv, struct pg_error **error);

/**
 * Run a benchmark on a brick.
 * Some benchmark needs one input brick and one output brick.
 *
 * @param   bench benchmark configuration to run.
 * @param   result benchmark results. No need to initialize it.
 * @param   error is set in case of an error
 * @return  0 on success, -1 on error.
 */
int pg_bench_run(struct pg_bench *bench, struct pg_bench_stats *result,
		 struct pg_error **error);

/**
 * Print results of a benchmark using a specific format.
 *
 * @param   result results to print.
 */
void pg_bench_print(struct pg_bench_stats *result);

/**
 * Print results of a benchmark.
 *
 * @param   result results to print.
 * @param   output where to write the output. NULL will print to stdout.
 */
void pg_bench_print_default(struct pg_bench_stats *result);

/**
 * Print header of benchmark result in csv format.
 *
 * @param   output where to write the output. NULL will print to stdout.
 */
void pg_bench_print_csv_header(FILE *output);

/**
 * Print result of a benchmark in csv format.
 *
 * @param   name test name printed in csv format
 * @param   output where to write the output. NULL will print to stdout.
 */
void pg_bench_print_csv(struct pg_bench_stats *r);

#endif /* _PG_UTILS_BENCH_H */
