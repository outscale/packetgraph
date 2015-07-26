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
#include <net/ethernet.h>
#include <endian.h>
#include "printer.h"

void print_summary(void *data, size_t size, FILE *o)
{
	/* A lot more work TODO */
	fprintf(o, " [eth] ");
	struct ether_header *eth = (struct ether_header *) data;
	if (eth->ether_type == htobe16(ETHERTYPE_IP))
		fprintf(o, "[ip4] ");
	else if (eth->ether_type == htobe16(ETHERTYPE_IPV6))
		fprintf(o, "[ip6] ");
	else if (eth->ether_type == htobe16(ETHERTYPE_ARP))
		fprintf(o, "[arp] ");
	else
		fprintf(o, "[%u]", (unsigned int) eth->ether_type);
	fprintf(o, "\n");
}

void print_raw(void *data, size_t size, FILE* o)
{
	size_t start=0;
	size_t end=0;

	while (end < size)
	{
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
