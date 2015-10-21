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

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <endian.h>
#include "printer.h"

static void print_l4(uint8_t type, void *data, size_t size, FILE *o);

static void print_proto_tcp(void *data, size_t size, FILE *o)
{
	struct tcphdr *h = (struct tcphdr *)data;

	if (size < sizeof(struct tcphdr))
		return;
	/* A lot more work TODO */
	fprintf(o, " [tcp sport=%u dport=%u]",
		be16toh(h->source), be16toh(h->dest));
}

static void print_proto_udp(void *data, size_t size, FILE *o)
{
	struct udphdr *h = (struct udphdr *)data;

	if (size < sizeof(struct udphdr))
		return;
	/* A lot more work TODO */
	fprintf(o, " [udp sport=%u dport=%u]",
		be16toh(h->source), be16toh(h->dest));
}

static void print_proto_icmp(void *data, size_t size, FILE *o)
{
	/* A lot more work TODO */
	fprintf(o, " [icmp]");
}

static void print_proto_icmpv6(void *data, size_t size, FILE *o)
{
	/* A lot more work TODO */
	fprintf(o, " [icmpv6]");
}

static void print_proto_igmp(void *data, size_t size, FILE *o)
{
	/* A lot more work TODO */
	fprintf(o, " [igmp]");
}

static void print_proto_ipv6_ext_hbh(void *data, size_t size, FILE *o)
{
	struct ip6_ext *ext = (struct ip6_ext *)data;
	uint8_t header_size = (ext->ip6e_len + 1) * 8;

	if (size < header_size)
		return;
	fprintf(o, " [ext Hop-by-Hop]");

	data = &((uint8_t *)(data))[header_size];
	print_l4(ext->ip6e_nxt, data, size - header_size, o);
}

static void print_proto_ipv6_ext_routing(void *data, size_t size, FILE *o)
{
	struct ip6_ext *ext = (struct ip6_ext *)data;
	uint8_t header_size = (ext->ip6e_len + 1) * 8;

	if (size < 16)
		return;
	fprintf(o, " [ext Routing]");
	data = &((uint8_t *)(data))[header_size];
	print_l4(ext->ip6e_nxt, data, size - header_size, o);
}

static void print_proto_ipv6_ext_fragment(void *data, size_t size, FILE *o)
{
	struct ip6_ext *ext = (struct ip6_ext *)data;
	uint8_t header_size = 8;

	if (size < header_size)
		return;
	fprintf(o, " [ext Fragment]");
	data = &((uint8_t *)(data))[header_size];
	print_l4(ext->ip6e_nxt, data, size - header_size, o);
}

static void print_proto_ipv6_ext_esp(void *data, size_t size, FILE *o)
{
	fprintf(o, " [ext ESP]");
}

static void print_proto_ipv6_ext_ah(void *data, size_t size, FILE *o)
{
	fprintf(o, " [ext AH]");
}

static void print_proto_ipv6_ext_destination(void *data, size_t size, FILE *o)
{
	struct ip6_ext *ext = (struct ip6_ext *)data;
	uint8_t header_size = (ext->ip6e_len + 1) * 8;

	if (size < header_size)
		return;
	fprintf(o, " [ext Destination]");
	data = &((uint8_t *)(data))[header_size];
	print_l4(ext->ip6e_nxt, data, size - header_size, o);
}

static void print_proto_ipv6_ext_mobility(void *data, size_t size, FILE *o)
{
	struct ip6_frag *ext = (struct ip6_frag *)data;
	uint8_t header_size = sizeof(struct ip6_frag);

	if (size < header_size)
		return;
	fprintf(o, " [ext Fragment]");
	data = &((uint8_t *)(data))[header_size];
	print_l4(ext->ip6f_nxt, data, size - header_size, o);
}

static void print_l4(uint8_t type, void *data, size_t size, FILE *o)
{
	switch (type) {
	case IPPROTO_TCP:
		print_proto_tcp(data, size, o);
		break;
	case IPPROTO_UDP:
		print_proto_udp(data, size, o);
		break;
	case IPPROTO_ICMP:
		print_proto_icmp(data, size, o);
		break;
	case IPPROTO_ICMPV6:
		print_proto_icmpv6(data, size, o);
		break;
	case IPPROTO_IGMP:
		print_proto_igmp(data, size, o);
		break;
	case 0x00:
		print_proto_ipv6_ext_hbh(data, size, o);
		break;
	case 0x2b:
		print_proto_ipv6_ext_routing(data, size, o);
		break;
	case 0x2c:
		print_proto_ipv6_ext_fragment(data, size, o);
		break;
	case 0x32:
		print_proto_ipv6_ext_esp(data, size, o);
		break;
	case 0x33:
		print_proto_ipv6_ext_ah(data, size, o);
		break;
	case 0x3c:
		print_proto_ipv6_ext_destination(data, size, o);
		break;
	case 0x87:
		print_proto_ipv6_ext_mobility(data, size, o);
		break;
	default:
		fprintf(o, " [type=%02x]", type);
		break;
	}
}

static void print_proto_ip(void *data, size_t size, FILE *o)
{
	struct ip *ip = (struct ip *) data;
	char sip[INET_ADDRSTRLEN];
	char dip[INET_ADDRSTRLEN];
	void *payload = (void *)(ip + 1);
	size_t payload_size = size - sizeof(struct ip);
	uint16_t payload_type = ip->ip_p;

	inet_ntop(AF_INET, (const void *) &(ip->ip_src.s_addr), sip,
		  INET_ADDRSTRLEN);
	inet_ntop(AF_INET, (const void *) &(ip->ip_dst.s_addr), dip,
		  INET_ADDRSTRLEN);
	fprintf(o, " [ip src=%s dst=%s]", sip, dip);

	print_l4(payload_type, payload, payload_size, o);
}

static void print_proto_ipv6(void *data, size_t size, FILE *o)
{
	struct ip6_hdr *ip = (struct ip6_hdr *) data;
	char sip[INET6_ADDRSTRLEN];
	char dip[INET6_ADDRSTRLEN];
	void *payload = (void *)(ip + 1);
	size_t payload_size = size - sizeof(struct ip6_hdr);
	uint8_t payload_type = ip->ip6_ctlun.ip6_un1.ip6_un1_nxt;

	inet_ntop(AF_INET6, (const void *) &(ip->ip6_src), sip,
		  INET6_ADDRSTRLEN);
	inet_ntop(AF_INET6, (const void *) &(ip->ip6_dst), dip,
		  INET6_ADDRSTRLEN);
	fprintf(o, " [ipv6 src=%s dst=%s]", sip, dip);
	print_l4(payload_type, payload, payload_size, o);
}

static void print_proto_arp(void *data, size_t size, FILE *o)
{
	/* A lot more work TODO */
	fprintf(o, " [arp]");
}

static void print_l3(uint16_t type, void *data, size_t size, FILE *o)
{
	uint16_t t = be16toh(type);

	switch (t) {
	case ETHERTYPE_IP:
		print_proto_ip(data, size, o);
		break;
	case ETHERTYPE_IPV6:
		print_proto_ipv6(data, size, o);
		break;
	case ETHERTYPE_ARP:
		print_proto_arp(data, size, o);
		break;
	default:
		fprintf(o, " [ethertype: %04x]", t);
		break;
	}
}

static void print_proto_eth(void *data, size_t size, FILE *o)
{
	struct ether_header *eth = (struct ether_header *) data;
	uint8_t *sm = eth->ether_shost;
	uint8_t *dm = eth->ether_dhost;
	void *payload = (void *)(eth + 1);
	size_t payload_size = size - sizeof(struct ether_header);
	uint16_t payload_type = eth->ether_type;

	fprintf(o, " [eth ");
	fprintf(o, "src=%02x:%02x:%02x:%02x:%02x:%02x ",
		sm[0], sm[1], sm[2], sm[3], sm[4], sm[5]);
	fprintf(o, "dst=%02x:%02x:%02x:%02x:%02x:%02x",
		dm[0], dm[1], dm[2], dm[3], dm[4], dm[5]);
	fprintf(o, "]");

	print_l3(payload_type, payload, payload_size, o);
}

void print_summary(void *data, size_t size, FILE *o)
{
	print_proto_eth(data, size, o);
	fprintf(o, "\n");
}

void print_raw(void *data, size_t size, FILE *o)
{
	size_t start = 0;
	size_t end = 0;

	while (end < size) {
		size_t i;

		end = start + 16;
		if (end >= size)
			end = size;

		fprintf(o, "%04x.  ", (uint16_t) start);
		for (i = start; i < end; i++) {
			fprintf(o, "%02x", ((uint8_t *)data)[i]);
			if ((i % 2 == 1) && (i != 0))
				fprintf(o, " ");
		}
		if (end == size) {
			size_t n = 16 - (size % 16);

			for (i = 0; i < n; i++) {
				fprintf(o, "  ");
			if ((i % 2 == 1) && (i != 0))
				fprintf(o, " ");
			}
			if (size % 2)
				fprintf(o, " ");
		}
		fprintf(o, "  ");
		for (i = start; i < end; i++) {
			uint8_t c = ((uint8_t *)data)[i];

			if (c < 0x20 || c > 0x7d)
				fprintf(o, ".");
			else
				fprintf(o, "%c", c);
		}
		fprintf(o, "\n");
		start = end;
	}
}
