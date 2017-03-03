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

#ifndef _PG_ANTISPOOF_H
#define _PG_ANTISPOOF_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

struct ether_addr;

/**
 * Create a new antispoof brick
 *
 * ARP antispoof is disabled by default
 *
 * @param	name name of the brick
 * @param	west_max maximum of links you can connect on the west side
 * @param	east_max maximum of links you can connect on the east side
 * @param	outside side where packets are not inspected
 * @param	mac valid pointer to a mac address that should not be spoofed
 * @param	errp is set in case of an error
 * @return	a pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_antispoof_new(const char *name,
				  enum pg_side outside,
				  struct ether_addr *mac,
				  struct pg_error **errp);

/**
 * Enable ARP anti-spoof
 *
 * Note that RARP is blocked if arp_antispoof is enabled.
 *
 * @param	brick pointer to an antispoof brick
 */
void pg_antispoof_arp_enable(struct pg_brick *brick);

/** Add authorized ip to antispoof brick
 *
 * @param	brick pointer to an antispoof brick
 * @param	ip IPv4 to associate with mac address
 * @param	errp is set in case of an error
 * @return	0 on success, -1 on error
 **/
int pg_antispoof_arp_add(struct pg_brick *brick, uint32_t ip,
			 struct pg_error **errp);

/** Remove authorized ip from antispoof brick
 *
 * @param	brick pointer to an antispoof brick
 * @param	ip IPv4 to unassociate with mac address
 * @param	errp is set in case of an error
 * @return	0 on success, -1 on error
 **/
int pg_antispoof_arp_del(struct pg_brick *brick, uint32_t ip,
			 struct pg_error **errp);

/** Remove all authorized ip from antispoof brick
 *
 * @param	brick pointer to an antispoof brick
 **/
void pg_antispoof_arp_del_all(struct pg_brick *brick);

/**
 * Disable ARP anti-spoof
 *
 * @param	brick pointer to an antispoof brick
 */
void pg_antispoof_arp_disable(struct pg_brick *brick);

/**
 * Enables Neighbor Discovery Protocol antispoof
 *
 * By enabling ND antispoof, the brick will check all ICMPv6 headers of a
 * packets looking Neighbor Advertisement emission from the inside.
 *
 * The brick will check that Target Address corresponds to one of allowed ipv6
 * and will check Target link-layer address option if it exists.
 * If Target link-layer address exists, it will check that provided mac address
 * corresponds to bricks's one.
 * https://tools.ietf.org/html/rfc4861#section-4.4
 *
 * @param	brick pointer to an antispoof brick
 */
void pg_antispoof_ndp_enable(struct pg_brick *brick);

/** Add authorized ip6 to antispoof brick
 *
 * @param	brick pointer to an antispoof brick
 * @param	ip IPv6 to associate with mac address (16 bytes)
 * @param	errp is set in case of an error
 * @return	0 on success, -1 on error
 **/
int pg_antispoof_ndp_add(struct pg_brick *brick, uint8_t *ip,
			struct pg_error **errp);

/** Remove authorized ip6 from antispoof brick
 *
 * @param	brick pointer to an antispoof brick
 * @param	ip IPv6 to unassociate with mac address (16 bytes)
 * @param	errp is set in case of an error
 * @return	0 on success, -1 on error
 **/
int pg_antispoof_ndp_del(struct pg_brick *brick, uint8_t *ip,
			struct pg_error **errp);

/** Remove all authorized ipv6 from antispoof brick
 *
 * @param	brick pointer to an antispoof brick
 **/
void pg_antispoof_ndp_del_all(struct pg_brick *brick);

/**
 * Disable Neighbor Discovery anti-spoof
 *
 * @param	brick pointer to an antispoof brick
 */
void pg_antispoof_ndp_disable(struct pg_brick *brick);

#endif  /* _PG_ANTISPOOF_H */
