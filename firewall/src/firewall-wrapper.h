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

#ifndef _FIREWALL_WRAPPER_H_
#define _FIREWALL_WRAPPER_H_

#ifdef NPF_GC_PATCHES
#include "npf_dpdk.h"

#define PG_NONE 0
#ifdef NPF_CONN_NO_THREADS
#define PG_NO_CONN_WORKER NPF_CONN_NO_THREADS
#else
#define PG_NO_CONN_WORKER 1 /* that sould not be usefull anyway */
#endif

static inline void pg_firewall_gc_internal(struct pg_brick *brick)
{
	struct pg_firewall_state *state;

	state = pg_brick_get_state(brick,
				   struct pg_firewall_state);
	npf_conn_call_gc(state->npf);
}

#define firewall_create(flags)			\
	(npf_dpdk_create(flags))

#else
#include "npf_dpdk.h"

#define NO_CONN_WORKER NPF_CONN_NO_THREADS

#define pg_firewall_gc_internal(firewall)	\
	(void)(firewall)

static inline npf_t *firewall_create(uint64_t flags)
{
	return npf_dpdk_create();
}


#endif

#endif
