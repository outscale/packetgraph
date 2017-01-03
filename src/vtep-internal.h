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

struct eth_ip_l4 {
	struct ether_hdr ethernet;
	union {
		struct {
			struct ipv4_hdr ipv4;
			struct tcp_hdr v4tcp;
		} __attribute__((__packed__));
		struct {
			struct ipv6_hdr ipv6;
			struct tcp_hdr v6tcp;
		} __attribute__((__packed__));
	} __attribute__((__packed__));
} __attribute__((__packed__));

enum operation {
	MLD_SUBSCRIBE = 131,
	MLD_UNSUBSCRIBE = 132,
	IGMP_SUBSCRIBE = 0x16,
	IGMP_UNSUBSCRIBE = 0x17,
};

int pg_vtep4_add_mac(struct pg_brick *brick, uint32_t vni,
		     struct ether_addr *mac, struct pg_error **errp);
int pg_vtep6_add_mac(struct pg_brick *brick, uint32_t vni,
		     struct ether_addr *mac, struct pg_error **errp);

#endif /* VTEP_INTERNAL_H_ */
