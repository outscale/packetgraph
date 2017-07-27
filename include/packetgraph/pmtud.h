/* Copyright 2016 Outscale SAS
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

#ifndef _PG_PMTUD_H
#define _PG_PMTUD_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Create a new path MTU discovery brick
 *
 * @param   name ame of the brick
 * @param   output side where packets can exit without been check
 * @param   mtu_size allowed MTU size (ethernet and protocols above)
 * @param   errp is set in case of an error
 * @return  a pointer to a brick, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_pmtud_new(const char *name,
			      enum pg_side output,
			      uint32_t mtu_size,
			      struct pg_error **errp);

#endif  /* _PG_PMTUD_H */
