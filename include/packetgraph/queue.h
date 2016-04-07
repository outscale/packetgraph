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

#ifndef _PG_QUEUE_H
#define _PG_QUEUE_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Create a new queue brick
 *
 * Queues are made to temporary keep a burst of packets waiting to be polled.
 * Queue bricks are pollable like a nic brick but get it's burst of packets from
 * an other "friend" brick (an other queue).
 *
 * This allow several graph to connect each other in a thead-safe maner.
 * A queue brick only have one pole (like nic bricks).
 *
 * To get a working queue, you will need two friend queues, example:
 *
 * [A]----[Queue1] ~ [Queue2]----[B]
 *
 * - [A] is linked to [Queue1], same for [Queue2] and [B].
 * - [Queue1] and [Queue2] are friend.
 * - [A] can burst packets to [Queue1] which store burst and returns.
 * - later, [Queue2] is polled. It get burst from [Queue1] and burst to [B].
 *
 * This way, [A] and [Queue1] can run in a separate thread than [Queue2] and [B]
 * To make [Queue1] and [Queue2] friends, see pg_queue_friend().
 *
 * Queues have a maximal size, if you try to burst in a full queue, the oldest
 * burst will be free.
 *
 * @name:	name of the brick
 * @size:	maximal size of the queue. If the queue is full, old bursts
 *		will be dropped. If size <= 0, a default queue size of 10 will
 *		be chosen.
 * @error:	is set in case of an error
 * @return:	a pointer to a brick structure, on success, NULL on error
 */
struct pg_brick *pg_queue_new(const char *name, int size,
			      struct pg_error **error);

/**
 * Make two queues friend together.
 *
 * @queue_1:	first queue to friend
 * @queue_2:	second queue to friend
 * @error:	is set in case of an error
 * @return:	0 on success, -1 on error
 */
int pg_queue_friend(struct pg_brick *queue_1,
		    struct pg_brick *queue_2,
		    struct pg_error **error);

/**
 * Check if two queues are friend or not
 *
 * @queue_1:	first queue to test
 * @queue_2:	the other queue to test
 * @return:	true if queues are friend, false otherwise
 */
bool pg_queue_are_friend(struct pg_brick *queue_1,
			 struct pg_brick *queue_2);

/**
 * Get friend if exists.
 * @queue:	queue who potentially have a friend
 * @return:	pointer to friend queue or NULL if have no friend
 */
struct pg_brick *pg_queue_get_friend(struct pg_brick *queue);

/**
 * Unfriend two queues.
 * @queue:	queue to unfriend, the other queue will be unfriend too
 */
void pg_queue_unfriend(struct pg_brick *queue);

/**
 * Get queue pressure of bursting queue.
 *
 * @queue:	queue to analyse
 * @return:	range [0, 255]. 0: empty queue, 255: full queue.
 */
uint8_t pg_queue_pressure(struct pg_brick *queue);

#endif  /* _PG_QUEUE_H */
