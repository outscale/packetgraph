/* Copyright 2014-2015 Nodalink EURL
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

#ifndef PG_UTILS_MAC_H
#define PG_UTILS_MAC_H

#include <stdbool.h>
#include <rte_ether.h>

union pg_mac {
	uint64_t mac;
	uint8_t bytes[8];
	uint16_t bytes16[4];
	uint32_t bytes32[2];
	struct {
		uint16_t padding1;
		uint8_t padding2;
		uint32_t part2;
	} __attribute__ ((__packed__));
} __attribute__ ((__packed__));

bool pg_scan_ether_addr(struct ether_addr *eth_addr, const char *buf);

void pg_set_mac_addrs(struct rte_mbuf *mb, const char *src, const char *dst);

void pg_set_ether_type(struct rte_mbuf *mb, uint16_t ether_type);

void pg_get_ether_addrs(struct rte_mbuf *mb, struct ether_hdr **dest);

void pg_print_mac(struct ether_addr *eth_addr);

const char *pg_printable_mac(struct ether_addr *eth_addr, char *buf);

#endif /* _PG_UTILS_MAC_H */
