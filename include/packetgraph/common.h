/* Copyright 2014 Nodalink EURL
 *
 * This file is part of Packetgraph.
 *
 * Packetgraph is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * Packetgraph is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Packetgraph.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PG_COMMON_H
#define _PG_COMMON_H

#include <sys/prctl.h>
#include <linux/seccomp.h>

enum pg_side {
	PG_WEST_SIDE = 0,
	PG_EAST_SIDE = 1,
	PG_MAX_SIDE  = 2
};

enum pg_mbuf_metaflag {
	PG_FRAGMENTED_MBUF = 1
};

static inline const char *pg_side_to_string(enum pg_side side)
{
	static const char * const sides[] = {"PG_WEST_SIDE",
					     "PG_EAST_SIDE",
					     "PG_MAX_SIDE"};

	return sides[side];
}

#define PG_DEFAULT_SIDE PG_WEST_SIDE

/* do not change this */
#define PG_MAX_PKTS_BURST	64

/* ignore new typedefs errors with checkpatch */
#define IGNORE_NEW_TYPEDEFS

/**
 * Revert side: PG_WEST_SIDE become PG_EAST_SIDE and vice versa.
 */
static inline enum pg_side pg_flip_side(enum pg_side side)
{
	return (enum pg_side)(side ^ 1);
}

/* Be sure to have result used. */
#define PG_WARN_UNUSED __attribute__((warn_unused_result))

/**
 * Initialize syscall filter
 *
 * @return 0 if the filter has been correctly build, -1 on the contrary.
 */
int pg_init_seccomp(void);

#endif /* _PG_COMMON_H */
