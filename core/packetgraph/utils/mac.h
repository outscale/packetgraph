/* Copyright 2014-2015 Nodalink EURL

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

#ifndef PG_CORE_UTILS_MAC_H
#define PG_CORE_UTILS_MAC_H

#include <rte_ether.h>

int pg_scan_ether_addr(struct ether_addr *eth_addr, const char *buf);

void pg_set_mac_addrs(struct rte_mbuf *mb, const char *src, const char *dst);

void pg_set_ether_type(struct rte_mbuf *mb, uint16_t ether_type);

void pg_get_ether_addrs(struct rte_mbuf *mb, struct ether_hdr **dest);

void pg_print_mac(struct ether_addr *eth_addr);

const char *pg_printable_mac(struct ether_addr *eth_addr, char *buf);

#endif /* _PG_CORE_UTILS_MAC_H */
