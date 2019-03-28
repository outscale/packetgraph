/* Copyright 2019 Outscale SAS
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

/**
 * WARNING THIS BRICK HASN'T BEEN TEST PROPERLY AND IS STILL EXPERIMENTALxs
 */

#ifndef _PG_ACCUMULATOR_H
#define _PG_ACCUMULATOR_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Create a new accumulator brick
 *
 * @param   name brick's name
 * @param   output side where packets will exit
 * @param   poll_min minimum number of polls to perform
 * @param   errp is set in case of an error
 * @return  a pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_accumulator_new(const char *name,
			      enum pg_side output,
			      uint64_t poll_min,
			      struct pg_error **errp);

#endif  /* _PG_ACCUMULATOR_H */
