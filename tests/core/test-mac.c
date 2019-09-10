/* Copyright 2014 Nodalink EURL
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

#include <stdio.h>
#include <memory.h>
#include <unistd.h>

#include <errno.h>
#include <glib.h>
#include <rte_mbuf.h>

#include "utils/tests.h"
#include "utils/mac.h"
#include "tests.h"
#include "utils/mempool.h"

static void test_print_mac(void)
{
	/* Build a MAC address for testing. */
	struct ether_addr addr = {{0x01, 0x22, 0xF1, 0x07, 0xA1, 0x01}};

	/* Redirect stdout to buffer. */
	char buffer[BUFSIZ] = {0};
	int out_pipe[2];
	int saved_stdout;
	/* Save default stdout to recover later. */
	saved_stdout = dup(STDOUT_FILENO);
	/* Do the redirection. */
	pipe(out_pipe);
	dup2(out_pipe[1], STDOUT_FILENO);
	close(out_pipe[1]);
	/* Anything sent to printf now go down the pipe
	 *So here we test pg_print_mac.
	 */
	pg_print_mac(&addr);
	/* Force flush/ */
	fflush(stdout);
	/* Read from pipe into buffer. */
	read(out_pipe[0], buffer, BUFSIZ);
	/* Reconnect stdout to terminal. */
	dup2(saved_stdout, STDOUT_FILENO);

	/* Now check the result. */
	g_assert(!strcmp(buffer, "01:22:F1:07:A1:01"));

}

static void test_printable_mac(void)
{
	/* Build a MAC adress for testing. */
	struct ether_addr addr = {{0x01, 0x22, 0xF1, 0x07, 0xA1, 0x01}};
	char tmp[18];

	/* Test printable_mac and check the result. */
	pg_printable_mac(&addr, tmp);
	g_assert(!strcmp(tmp, "01:22:F1:07:A1:01"));
}

static void test_scan_ether_addr(void)
{
	/* Build a MAC adress for testing. */
	struct ether_addr addr = {{0x01, 0x22, 0xF1, 0x07, 0xA1, 0x01}};

	/* Test scan_ether_addr with a too small buffer to raise the error
	 * and then check the result. */
	g_assert(pg_scan_ether_addr(&addr, "") == false);
}

static void test_set_get_ether_tpe_and_addrs(void)
{
	struct rte_mbuf *mb;
	struct ether_hdr *hdr;
	uint16_t ether_tpe = 0x0800;

	/* Create the packet struct. */
	mb = rte_pktmbuf_alloc(pg_get_mempool());

	/* Test pg_set_ether_type. */
	pg_set_ether_type(mb, ether_tpe);
	hdr = rte_pktmbuf_mtod(mb, struct ether_hdr*);
	/* Test pg_get_ether_addrs. */
	pg_get_ether_addrs(mb, &hdr);
	/* Check if ether_tpe send as input is at the output. */
	g_assert(rte_be_to_cpu_16(hdr->ether_type) ==  ether_tpe);
	/* Check other outputs of get_ether_addrs.
	 * So first we set the two address up. */
	pg_set_mac_addrs(mb, "AA:09:69:FF:AC:00", "BB:09:69:FF:AC:00");
	/* Then, check. */
	g_assert(!memcmp(&hdr->s_addr,
		(char []) {0xAA,0x09,0x69,0xFF,0xAC,0x00,0}, 6));
	g_assert(!memcmp(&hdr->d_addr,
		(char []) {0xBB,0x09,0x69,0xFF,0xAC,0x00,0}, 6));
	/* Now free the packet. */
	rte_pktmbuf_free(mb);
}

void test_mac(void)
{
	pg_test_add_func("/core/mac/print_mac", test_print_mac);
	pg_test_add_func("/core/mac/printable_mac", test_printable_mac);
	pg_test_add_func("/core/mac/scan_ether_addr", test_scan_ether_addr);
	pg_test_add_func("/core/mac/set_get_ether_tpe_and_addrs",
			 test_set_get_ether_tpe_and_addrs);
}
