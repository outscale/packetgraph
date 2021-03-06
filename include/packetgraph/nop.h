/* Copyright 2015 Outscale SAS
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

#ifndef _PG_NOP_H
#define _PG_NOP_H

#include <packetgraph/errors.h>

/**
 * This is a dummy brick who don't do anything useful.
 * It's purpose is to show an example of brick to new developpers.
 */
PG_WARN_UNUSED
struct pg_brick *pg_nop_new(const char *name,
			    struct pg_error **errp);

#endif  /* _PG_NOP_H */


