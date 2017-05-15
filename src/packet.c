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
 */

#include <rte_config.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <packetgraph/common.h>
#include <packetgraph/packet.h>

uint8_t *pg_packet_data(pg_packet_t *packet)
{
	return rte_pktmbuf_mtod(packet, uint8_t *);
}

unsigned int pg_packet_len(pg_packet_t *packet)
{
	return rte_pktmbuf_pkt_len(packet);
}

unsigned int pg_packet_max_len(pg_packet_t *packet)
{
	return packet->buf_len - rte_pktmbuf_headroom(packet);
}

int pg_packet_set_len(pg_packet_t *packet, unsigned int len)
{
	if (len > pg_packet_max_len(packet))
		return -1;
	packet->pkt_len = len;
	packet->data_len = len;
	return 0;
}

uint16_t pg_packet_ipv4_checksum(uint8_t *ip_hdr)
{
	return rte_ipv4_cksum((struct ipv4_hdr *) ip_hdr);
}

void pg_packet_set_l2_len(pg_packet_t *packet, unsigned int len)
{
	packet->l2_len = len;
}

int pg_packet_get_l2_len(pg_packet_t *packet)
{
	return packet->l2_len;
}

void pg_packet_set_l3_len(pg_packet_t *packet, unsigned int len)
{
	packet->l3_len = len;
}

int pg_packet_get_l3_len(pg_packet_t *packet)
{
	return packet->l3_len;
}

void pg_packet_set_l4_len(pg_packet_t *packet, unsigned int len)
{
	packet->l4_len = len;
}

int pg_packet_get_l4_len(pg_packet_t *packet)
{
	return packet->l4_len;
}
