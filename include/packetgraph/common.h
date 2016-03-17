/* Copyright 2014 Nodalink EURL
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

#ifndef _PG_CORE_COMMON_H
#define _PG_CORE_COMMON_H

enum pg_side {
	WEST_SIDE = 0,
	EAST_SIDE = 1,
	MAX_SIDE  = 2
};

static inline const char *pg_side_to_string(enum pg_side side)
{
	static const char * const sides[] = {"WEST_SIDE",
					     "EAST_SIDE",
					     "MAX_SIDE"};

	return sides[side];
}

#define DEFAULT_SIDE WEST_SIDE

/* do not change this */
#define PG_MAX_PKTS_BURST	64

#endif /* _PG_CORE_COMMON_H */
