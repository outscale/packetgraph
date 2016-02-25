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


#ifndef _PG_CORE_COLLECT_H
#define _PG_CORE_COLLECT_H

#include <packetgraph/utils/errors.h>

struct pg_brick *pg_collect_new(const char *name, uint32_t west_max,
				uint32_t east_max,
				struct pg_error **errp);

#endif  /* _PG_CORE_COLLECT_H */
