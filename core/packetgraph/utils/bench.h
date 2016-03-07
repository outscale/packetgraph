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

#ifndef _PG_CORE_UTILS_BENCH_H
#define _PG_CORE_UTILS_BENCH_H

#include <sys/time.h>
#include <stdbool.h>
#include <packetgraph/common.h>
#include <packetgraph/brick.h>

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
	/* A nop brick is connected to output_brick in order to count
	 * incomming packets. If you set count_brick to NULL, a nop brick
	 * will be linked during the benchmark. If you set your own brick,
	 * You will be responsible to link it to your output_brick before
	 * launching a benchmark.
	 */
	struct pg_brick *count_brick;
	void (*post_burst_op)(struct pg_bench *);
};

/**
 * @param  bench struct pg_bench * to init
 */
#define pg_bench_init(bench)				\
	(memset((bench), 0, sizeof(struct pg_bench)))

/**
 * Run a benchmark on a brick.
 * Some benchmark needs one input brick and one output brick.
 *
 * @param   bench benchmark configuration to run.
 * @param   result benchmark results. No need to initialize it.
 * @param   error is set in case of an error
 * @return  1 on success, 0 on error.
 */
int pg_bench_run(struct pg_bench *bench, struct pg_bench_stats *result,
		 struct pg_error **error);

/**
 * Print results of a benchmark.
 *
 * @param   result results to print.
 * @param   output where to write the output. NULL will print to stdout.
 * @return  1 on success, 0 on error.
 */
bool pg_bench_print(struct pg_bench_stats *result, FILE *output);

#endif /* _PG_CORE_UTILS_BENCH_H */
