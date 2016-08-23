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

#ifndef _PG_VTEP_H
#define _PG_VTEP_H

#include <packetgraph/packetgraph.h>
#include <packetgraph/errors.h>

enum pg_vtep_flags {
	NONE = 0,
	/* modify the packets instead of copy it */
	NO_COPY = 1,
	/* don't check if the VNI has the coresponding mac,
	 * when incoming packets, just forward it */
	NO_INNERMAC_CKECK = 2,
	ALL_OPTI = NO_COPY | NO_INNERMAC_CKECK
};

struct ether_addr;

/**
 * Get MAC address of the vtep
 * @param	brick a pointer to a vtep brick
 * @return	mac address of the vtep
 */
struct ether_addr *pg_vtep_get_mac(struct pg_brick *brick);

/**
 * Add a VNI to the VTEP
 *
 * NOTE: Adding the same VNI twice is not authorized and will result in an
 *       assertion
 *
 * @param	brick the brick we are working on
 * @param	neighbor a brick connected to the VTEP port
 * @param	vni the VNI to add
 * @param	multicast_ip the multicast ip to associate to the VNI
 * @param	errp an error pointer
 */
int pg_vtep_add_vni(struct pg_brick *brick,
		    struct pg_brick *neighbor,
		    uint32_t vni, uint32_t multicast_ip,
		    struct pg_error **errp);

/**
 * Add a MAC to the VTEP VNI
 *
 * @param	brick the brick we are working on
 * @param	vni the vni on which you want to add @mac
 * @param	mac the mac
 * @param	errp an error pointer
 */
int pg_vtep_add_mac(struct pg_brick *brick, uint32_t vni,
		    struct ether_addr *mac, struct pg_error **errp);

/**
 * Create a new vtep
 *
 * @name:	the name
 * @west_max:	the maximum number of connections to the west
 * @east_max:	The maximum number of connections to the east
 * @output:	Side where packets are encapsuled in VXLAN
 * @ip:		the vtep ip
 * @mac:	the vtep mac
 * @flag:	the vtep flag options(see pg_vtep_flags)
 * @errp:	an error pointer
 * @return	a pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_vtep_new(const char *name, uint32_t west_max,
			     uint32_t east_max, enum pg_side output,
			     uint32_t ip, struct ether_addr mac,
			     int flag, struct pg_error **errp);
#endif /* _PG_VTEP_H */
