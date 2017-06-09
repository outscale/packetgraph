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
 */

#ifndef _PG_DHCP_H
#define _PG_DHCP_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>


/**
 *  Create a new dhcp brick.
 *  dhcp brick allow ip adress to mac adresses
 *  @name: name of the brick
 *  @cidr: range of ip adresses
 *  @errp: set in case of an error
 *  @return: a pointer to a brick structure on success, NULL on error
 **/

struct pg_brick *pg_dhcp_new(const char *name, const char *ipcidr,
			     enum pg_side outside, struct pg_error **errp);

