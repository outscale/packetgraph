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
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <endian.h>
#include "printer.h"

static void print_proto_tcp(void *data, size_t size, FILE *o)
{
	/* A lot more work TODO */
	fprintf(o, " [tcp] ");
}

static void print_proto_udp(void *data, size_t size, FILE *o)
{
	/* A lot more work TODO */
	fprintf(o, " [udp] ");
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

	if (payload_type == IPPROTO_TCP)
		print_proto_tcp(payload, payload_size, o);
	else if (payload_type == IPPROTO_UDP)
		print_proto_udp(payload, payload_size, o);
	else
		fprintf(o, " [type=%x]", payload_type);
}

static void print_proto_arp(void *data, size_t size, FILE *o)
{
	/* A lot more work TODO */
	fprintf(o, " [arp] ");
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

	if (payload_type == htobe16(ETHERTYPE_IP))
		print_proto_ip(payload, payload_size, o);
	else if (payload_type == htobe16(ETHERTYPE_ARP))
		print_proto_arp(payload, payload_size, o);
	else
		fprintf(o, " [ethertype: %x]", (unsigned int) eth->ether_type);
	fprintf(o, "\n");
}

void print_summary(void *data, size_t size, FILE *o)
{
	print_proto_eth(data, size, o);
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
