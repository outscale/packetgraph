/* Copyright 2017 Outscale SAS
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
 *
 * The multicast_subscribe and multicast_unsubscribe functions
 * are modifications of igmpSendReportMessage/igmpSendLeaveGroupMessage from
 * CycloneTCP Open project.
 * Copyright 2010-2015 Oryx Embedded SARL.(www.oryx-embedded.com)
 */

#ifndef VTEP_INTERNAL_H_
#define VTEP_INTERNAL_H_

struct vtep_state;
struct vtep_port;
struct dest_addresses;

enum operation {
	MLD_SUBSCRIBE = 131,
	MLD_UNSUBSCRIBE = 132,
	IGMP_SUBSCRIBE = 0x16,
	IGMP_UNSUBSCRIBE = 0x17,
};


void pg_vtep4_clean_all_mac(struct pg_brick *v, int port_id);
void pg_vtep6_clean_all_mac(struct pg_brick *v, int port_id);

int pg_vtep4_add_mac(struct pg_brick *brick, uint32_t vni,
		     struct ether_addr *mac, struct pg_error **errp);
int pg_vtep6_add_mac(struct pg_brick *brick, uint32_t vni,
		     struct ether_addr *mac, struct pg_error **errp);

struct pg_mac_table *pg_vtep4_mac_to_dst(struct pg_brick *brick,
					 int port_id);
struct pg_mac_table *pg_vtep6_mac_to_dst(struct pg_brick *brick,
					 int port_id);

int pg_vtep4_unset_mac(struct pg_brick *brick, uint32_t vni,
		       struct ether_addr *mac, struct pg_error **errp);
int pg_vtep6_unset_mac(struct pg_brick *brick, uint32_t vni,
		       struct ether_addr *mac, struct pg_error **errp);

/*
 * headers_v4/6 and dest_addresses_v4 are not use in any vtep implementations
 * but are here for tests
 */
struct headers_v4 {
	struct ipv4_hdr	 ip; /* define in rte_ip.h */
	struct udp_hdr	 udp; /* define in rte_udp.h */
	struct vxlan_hdr vxlan; /* define in rte_ether.h */
} __attribute__((__packed__));

struct headers_v6 {
	struct ipv6_hdr	 ip; /* define in rte_ip.h */
	struct udp_hdr	 udp; /* define in rte_udp.h */
	struct vxlan_hdr vxlan; /* define in rte_ether.h */
} __attribute__((__packed__));

struct dest_addresses_v4 {
	struct ether_addr mac;
	uint32_t ip;
	uint64_t lifetime;
};


#endif /* VTEP_INTERNAL_H_ */
