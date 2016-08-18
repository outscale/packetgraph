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

#ifndef _PG_TAP_H
#define _PG_TAP_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Create a new TAP brick.
 * This will create a new classic tap kernel interface.
 * If you want a faster interface, you should look at special dpdk interfaces
 * KNI (http://dpdk.org/doc/guides/prog_guide/kernel_nic_interface.html)
 *
 * @name:	name of the brick
 * @ifname:	interface name, set to NULL to automatically get an name.
		If provided, interface name must have a max size of IFNAMSIZ.
		Larger names will be truncated.
 * @errp:	is set in case of an error
 * @return:	a pointer to a brick structure on success, 0 on error
 */
struct pg_brick *pg_tap_new(const char *name,
			    const char *ifname,
			    struct pg_error **errp);

/**
 * Get interface's name.
 *
 * @brick:	brick's pointer
 * @return:	a pointer to interface's name (you MUST NOT free it)
 */
const char *pg_tap_ifname(struct pg_brick *brick);

/** get the mac address of the tap brick
 *
 * @param tap	a pointer to a tap brick
 * @param addr	a pointer to a ether_addr structure to to filled with the
 *		Ethernet address
 * @return	0 on success, -1 on error
 */
int pg_tap_get_mac(struct pg_brick *tap, struct ether_addr *addr);

#endif  /* _PG_TAP_H */
