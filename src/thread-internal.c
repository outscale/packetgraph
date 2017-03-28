/* Copyright 2017 Outscale SAS
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
 *
 * The multicast_subscribe and multicast_unsubscribe functions
 * are modifications of igmpSendReportMessage/igmpSendLeaveGroupMessage from
 * CycloneTCP Open project.
 * Copyright 2010-2015 Oryx Embedded SARL.(www.oryx-embedded.com)
 */

#ifndef TYPE_NAME
#error "TYPE_NAME is not define"
#endif

#define CAT(a, b) a ## b

#define ARG_TYPE_ptr void *
#define ARG_TYPE_i16 int16_t
#define ARG_TYPE_i32 int32_t

#define ARG_TYPE_(t) CAT(ARG_TYPE_, t)
#define ARG_TYPE ARG_TYPE_(TYPE_NAME)

#define enqueue__(version) CAT(enqueue_, version)
#define enqueue_ enqueue__(TYPE_NAME)

static int enqueue_(int16_t thread_id, enum pg_thread_op op, ARG_TYPE arg)
{
	int ret = -1;
	int queue_pos;

	while (!rte_atomic16_test_and_set(&thread_ids[thread_id]->
					  is_queue_locked))
		sched_yield();
	queue_pos = thread_ids[thread_id]->queue_size;
	if ((queue_pos + 1) == PG_THREAD_MAX_OP)
		goto exit;
	ret = queue_pos;
	thread_ids[thread_id]->queue_size += 1;
	thread_ids[thread_id]->queue[queue_pos].op = op;
	thread_ids[thread_id]->queue[queue_pos].arg.TYPE_NAME = arg;
exit:
	rte_atomic16_clear(&thread_ids[thread_id]->is_queue_locked);
	return ret;
}

#define wait_enqueue__(version) CAT(wait_enqueue_, version)
#define wait_enqueue_ wait_enqueue__(TYPE_NAME)

static int wait_enqueue_(int16_t thread_id, enum pg_thread_op op, ARG_TYPE arg)
{
	int i;

	while ((i = enqueue_(thread_id, op, arg)) < 0)
		sched_yield();
	return i;
}

#undef CAT
#undef enqueue__
#undef enqueue_
#undef wait_enqueue__
#undef wait_enqueue_
#undef ARG_TYPE_ptr
#undef ARG_TYPE_i16
#undef ARG_TYPE_i32
#undef ARG_TYPE_
#undef ARG_TYPE
