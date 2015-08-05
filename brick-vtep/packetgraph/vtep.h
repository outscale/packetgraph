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

#ifndef _BRICKS_BRICK_VTEP_H_
#define _BRICKS_BRICK_VTEP_H_

#ifndef __cplusplus
#include <rte_ether.h>
#else
struct ether_addr;
#endif

/**
 * Create a new vtep
 *
 * @name:	the name
 * @west_max:	the maximum number of connections to the west
 * @east_max:	The maximum number of connections to the east
 * @ip:		the vtep ip
 * @mac:	the vtep mac
 * @errp:	an error pointer
 */
struct brick *vtep_new(const char *name, uint32_t west_max,
		       uint32_t east_max, enum side output,
		       uint32_t ip, struct ether_addr mac,
		       struct switch_error **errp);

/**
 * Get MAC address of the vtep
 * @param	brick a pointer to a vtep brick
 * @return	mac address of the vtep
 */
struct ether_addr *vtep_get_mac(struct brick *brick);

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
void vtep_add_vni(struct brick *brick,
		  struct brick *neighbor,
		  uint32_t vni, uint32_t multicast_ip,
		  struct switch_error **errp);

#endif
