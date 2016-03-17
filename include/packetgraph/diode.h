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

#ifndef _PG_DIODE_DIODE_H
#define _PG_DIODE_DIODE_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Create a new diode brick
 *
 * @name:	name of the brick
 * @west_max:	maximum of links you can connect on the west side
 * @east_max:	maximum of links you can connect on the east side
 * @output:	side where packets can exit (they come from the opposite side)
 * @errp:	is set in case of an error
 * @return:	a pointer to a brick structure, on success, 0 on error
 */
struct pg_brick *pg_diode_new(const char *name,
			      uint32_t west_max,
			      uint32_t east_max,
			      enum pg_side output,
			      struct pg_error **errp);

#endif  /* _PG_DIODE_DIODE_H */
