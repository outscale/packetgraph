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
#include <arpa/inet.h>

/**
 *  Create a new dhcp brick.
 *  dhcp brick allow ip adress to mac adresses
 *  @name: name of the brick
 *  @cidr: range of ip adresses
 *  @errp: set in case of an error
 *  @return: a pointer to a brick structure on success, NULL on error
 **/

struct pg_brick *pg_dhcp_new(const char *name, const char *cidr,
			     struct ether_addr mac_dhcp,
			     struct pg_error **errp);

/**
 * Calculate the number of potential adresses
 * @ipcidr : ip adress with cidr to calculate the range
 */
int pg_calcul_range_ip(network_addr_t ipcidr);

/**
 * Check if a packet is a dhcp discover packet
 * @pkts : the packet to check
 */
bool is_discover(struct rte_mbuf *pkts);

/**
 * Check if a packet is a dhcp discover packet
 * @pkts : the packet to check
 */
bool is_request(struct rte_mbuf *pkts);

/**
 * Create dhcp packet to burst
 * @msg_type : dhcp message type : 1 : Discover
 * 				   2 : Offer
 * 				   3 : Request
 * 				   4 : Decline
 * 				   5 : Acknowledgment
 * 				   6 : Negative Acknowledgment 
 * 				   7 : Realease
 * 				   8 : Informationnal
 * @mac : Mac adresse of the brick
 * @ip_offer : ip to give to the brick
 * @bail : number of seconds for bail of the ip adresse to the brick
 * @ip_router : ip of the default gateaway
 * @mask : subnet mask used by the client
 */
struct rte_mbuf **pg_dhcp_packet_create(struct pg_brick *brick,
					uint8_t msg_type, struct ether_addr mac,
                                        uint32_t ip_offer, int bail,
                                        uint32_t ip_router, uint32_t mask);

#endif /* _PG_DHCP_H */
