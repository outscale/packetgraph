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
 *  on Linux:
 *   gcc -o iprange iprange.c -O2 -Wall
 *  on Solaris 8, Studio 8 CC:
 *   cc -xO5 -xarch=v8plusa -xdepend iprange.c -o iprange -lnsl -lresolv
 *
 * CHANGELOG:
 *  2004-10-16 Paul Townsend (alpha alpha beta at purdue dot edu)
 *   - more general input/output formatting
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <packetgraph/packetgraph.h>

static char	*PROG;

/*------------------------------------------------------------------*/
/* Set a bit to a given value (0 or 1); MSB is bit 1, LSB is bit 32 */
/*------------------------------------------------------------------*/
in_addr_t set_bit( in_addr_t addr, int bitno, int val ) {

  if ( val )
    return( addr | (1 << (32 - bitno)) );
  else
    return( addr & ~(1 << (32 - bitno)) );

} /* set_bit() */


/*--------------------------------------*/
/* Compute netmask address given prefix */
/*--------------------------------------*/
in_addr_t netmask( int prefix ) {

  if ( prefix == 0 )
    return( ~((in_addr_t) -1) );
  else
    return( ~((1 << (32 - prefix)) - 1) );

} /* netmask() */


/*----------------------------------------------------*/
/* Compute broadcast address given address and prefix */
/*----------------------------------------------------*/
in_addr_t broadcast( in_addr_t addr, int prefix ) {

  return( addr | ~netmask(prefix) );

} /* broadcast() */


/*--------------------------------------------------*/
/* Compute network address given address and prefix */
/*--------------------------------------------------*/
in_addr_t network( in_addr_t addr, int prefix ) {

  return( addr & netmask(prefix) );

} /* network() */


/*------------------------------------------------*/
/* Print out a 32-bit address in A.B.C.D/M format */
/*------------------------------------------------*/
void print_addr( in_addr_t addr, int prefix ) {

  struct in_addr in;

  in.s_addr = htonl( addr );
  if ( prefix < 32 )
    printf( "%s/%d\n", inet_ntoa(in), prefix );
  else
    printf( "%s\n", inet_ntoa(in));

} /* print_addr() */

/*-----------------------------------------------------------*/
/* Convert an A.B.C.D address into a 32-bit host-order value */
/*-----------------------------------------------------------*/
in_addr_t a_to_hl(const char *ipstr ) {
  struct in_addr in;
  if ( !inet_aton(ipstr, &in) ) {
    fprintf( stderr, "%s: Invalid address %s!\n", PROG, ipstr );
    exit( 1 );
  }

  return( ntohl(in.s_addr) );

} /* a_to_hl() */


/*-----------------------------------------------------------------*/
/* convert a network address char string into a host-order network */
/* address and an integer prefix value                             */
/*-----------------------------------------------------------------*/
int str_to_netaddr(const char *ipstr, network_addr_t *netaddr) {
  long int prefix = 32;
  char *prefixstr;
   
  if ( (prefixstr = strchr(ipstr, '/')) ) {
    *prefixstr = '\0';
    prefixstr++;
    prefix = strtol( prefixstr, (char **) NULL, 10 );
    if ((*prefixstr == '\0') || (prefix < 0) || (prefix > 32)) {
      fprintf( stderr, "%s: Invalid prefix /%s...!\n", PROG, prefixstr );
      return -1;
    }
  }
  else {
    printf("no prefix");
  }
  
  netaddr->pfx = (int) prefix;
  netaddr->addr = network( a_to_hl(ipstr), prefix );
  return 0;

} /* str_to_netaddr() */

/*------------------------------------------------------*/
/* Print out an address range in a.b.c.d-A.B.C.D format */
/*------------------------------------------------------*/
void print_addr_range( in_addr_t lo, in_addr_t hi ) {

  struct in_addr in;

  if ( lo != hi ) {
    in.s_addr = htonl( lo );
    printf( "%s-", inet_ntoa(in) );
  }

  in.s_addr = htonl( hi );
  printf( "%s\n", inet_ntoa(in) );

} /* print_addr_range() */

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

