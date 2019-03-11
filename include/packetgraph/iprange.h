/*
 * Copyright (C) 2003 Gabriel L. Somlo
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * To compile:
 * on Linux:
 * gcc -o iprange iprange.c -O2 -Wall
 * on Solaris 8, Studio 8 CC:
 * cc -xO5 -xarch=v8plusa -xdepend iprange.c -o iprange -lnsl -lresolv
 *
 * CHANGELOG :
 * 2004-10-16 Paul Townsend (alpha alpha beta at purdue dot edu)
 *     more general input/output formatting
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*---------------------------------------------------------------------*/
/* network address type: one field for the net address, one for prefix */
/*---------------------------------------------------------------------*/
typedef struct network_addr {
  in_addr_t addr;
  int pfx;
} network_addr_t;

/*------------------------------------------------------------------*/
/* Set a bit to a given value (0 or 1); MSB is bit 1, LSB is bit 32 */
/*------------------------------------------------------------------*/
in_addr_t set_bit(in_addr_t addr, int bitno, int val);

/*----------------------------------------------------*/
/* Compute broadcast address given address and prefix */
/*----------------------------------------------------*/
in_addr_t broadcast(in_addr_t addr, int prefix);

/*--------------------------------------*/
/* Compute netmask address given prefix */
/*--------------------------------------*/
in_addr_t netmask(int prefix);

/*------------------------------------------------*/
/* Print out a 32-bit address in A.B.C.D/M format */
/*------------------------------------------------*/
void print_addr(in_addr_t addr, int prefix);

/*-----------------------------------------------------------*/
/* Convert an A.B.C.D address into a 32-bit host-order value */
/*-----------------------------------------------------------*/
in_addr_t a_to_hl(const char *ipstr);

/*--------------------------------------------------*/
/* Compute network address given address and prefix */
/*--------------------------------------------------*/
in_addr_t network(in_addr_t addr, int prefix);

/*-----------------------------------------------------------------*/
/* convert a network address char string into a host-order network */
/* address and an integer prefix value                             */
/*-----------------------------------------------------------------*/
int str_to_netaddr(const char *ipstr, network_addr_t *netaddr);

/*------------------------------------------------------*/
/* Print out an address range in a.b.c.d-A.B.C.D format */
/*------------------------------------------------------*/
void print_addr_range(in_addr_t lo, in_addr_t hi);
