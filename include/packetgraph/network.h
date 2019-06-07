/* Copyright 2019 Outscale SAS
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

#ifndef PG_NETWORK_H_
#define PG_NETWORK_H_

#include <packetgraph/byteorder.h>

#ifndef ETHER_TYPE_IPv4 /* Assuming no ether type are define */
#define ETHER_TYPE_IPv4 0x0800 /**< IPv4 Protocol. */
#define ETHER_TYPE_IPv6 0x86DD /**< IPv6 Protocol. */
#define ETHER_TYPE_ARP  0x0806 /**< Arp Protocol. */
#define ETHER_TYPE_RARP 0x8035 /**< Reverse Arp Protocol. */
#define ETHER_TYPE_VLAN 0x8100 /**< IEEE 802.1Q VLAN tagging. */
#define ETHER_TYPE_QINQ 0x88A8 /**< IEEE 802.1ad QinQ tagging. */
#define ETHER_TYPE_ETAG 0x893F /**< IEEE 802.1BR E-Tag. */
#define ETHER_TYPE_1588 0x88F7 /**< IEEE 802.1AS 1588 Precise Time Protocol. */
#define ETHER_TYPE_SLOW 0x8809 /**< Slow protocols (LACP and Marker). */
#define ETHER_TYPE_TEB  0x6558 /**< Transparent Ethernet Bridging. */
#define ETHER_TYPE_LLDP 0x88CC /**< LLDP Protocol. */
#define ETHER_TYPE_MPLS 0x8847 /**< MPLS ethertype. */
#define ETHER_TYPE_MPLSM 0x8848 /**< MPLS multicast ethertype. */

#endif

#ifndef IPV4_HDR_DF_FLAG

#define	IPV4_HDR_DF_SHIFT	14
#define	IPV4_HDR_MF_SHIFT	13
#define	IPV4_HDR_FO_SHIFT	3

#define	IPV4_HDR_DF_FLAG	(1 << IPV4_HDR_DF_SHIFT)
#define	IPV4_HDR_MF_FLAG	(1 << IPV4_HDR_MF_SHIFT)
#endif

#define PG_BE_ETHER_TYPE_ARP PG_CPU_TO_BE_16(ETHER_TYPE_ARP)
#define PG_BE_ETHER_TYPE_RARP PG_CPU_TO_BE_16(ETHER_TYPE_RARP)
#define PG_BE_ETHER_TYPE_IPv4 PG_CPU_TO_BE_16(ETHER_TYPE_IPv4)
#define PG_BE_ETHER_TYPE_IPv6 PG_CPU_TO_BE_16(ETHER_TYPE_IPv6)
#define PG_BE_ETHER_TYPE_VLAN PG_CPU_TO_BE_16(ETHER_TYPE_VLAN)
#define PG_BE_IPV4_HDR_DF_FLAG PG_CPU_TO_BE_16(IPV4_HDR_DF_FLAG)

#define PG_ICMP_PROTOCOL 0x01


#endif
