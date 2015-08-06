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

#include <stdio.h>
#include <string.h>

#include <rte_config.h>
#include <rte_hash_crc.h>

#include <packetgraph/utils/mac.h>

inline int pg_scan_ether_addr(struct ether_addr *eth_addr, const char *buf)
{
	int ret;

	/* is the input string long enough */
	if (strlen(buf) < 17)
		return 0;

	ret = sscanf(buf, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
		&eth_addr->addr_bytes[0],
		&eth_addr->addr_bytes[1],
		&eth_addr->addr_bytes[2],
		&eth_addr->addr_bytes[3],
		&eth_addr->addr_bytes[4],
		&eth_addr->addr_bytes[5]);

	/* where the 6 bytes parsed ? */
	return ret == 6;
}

void pg_set_mac_addrs(struct rte_mbuf *mb, const char *src, const char *dst)
{
	struct ether_hdr *eth_hdr;

	eth_hdr = rte_pktmbuf_mtod(mb, struct ether_hdr *);

	pg_scan_ether_addr(&eth_hdr->s_addr, src);
	pg_scan_ether_addr(&eth_hdr->d_addr, dst);
}

void pg_set_ether_type(struct rte_mbuf *mb, uint16_t ether_type)
{
	struct ether_hdr *eth_hdr;

	eth_hdr = rte_pktmbuf_mtod(mb, struct ether_hdr *);

	eth_hdr->ether_type = rte_cpu_to_be_16(ether_type);
}

void pg_get_ether_addrs(struct rte_mbuf *mb, struct ether_hdr **dest)
{
	*dest = rte_pktmbuf_mtod(mb, struct ether_hdr *);
}

const char *pg_printable_mac(struct ether_addr *eth_addr, char *buf)
{
	ether_format_addr(buf, 32, eth_addr);
	return buf;
}

void pg_print_mac(struct ether_addr *eth_addr)
{
	char buf[32];

	ether_format_addr(buf, 32, eth_addr);
	printf("%s", buf);
}
