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

#ifndef _PG_NIC_H
#define _PG_NIC_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Start brick-nic library, must be call before any usage of this lib
 */
void pg_nic_start(void);

/**
 * Create a new nic brick
 *
 * @name:	name of the brick
 * @ifname:	the name of the interface you want to use.
 *	This is the same syntax as --vdev in DPDK:
 *	http://pktgen.readthedocs.org/en/latest/usage_eal.html?highlight=vdev
 *	For example, if you want to open a system NIC, put:
 *	"eth_pcap0,iface=eth2"
 * @errp:	set in case of an error
 * @return:	a pointer to a brick structure on success, NULL on error
 */
struct pg_brick *pg_nic_new(const char *name,
			    const char *ifname,
			    struct pg_error **errp);

/**
 * Create a new nic brick
 *
 * @name:	name of the brick
 * @output:	The side of the output (so the side of the nic)
 * @portid:	the id of the interface you want to use
 * @errp:	set in case of an error
 * @return:	a pointer to a brick structure, on success, 0 on error
 */
struct pg_brick *pg_nic_new_by_id(const char *name,
				  uint8_t portid,
				  struct pg_error **errp);

/**
 * Get number of available DPDK ports
 *
 * @return:	number of DPDK ports
 */
int pg_nic_port_count(void);

/**
 * Set a custom MTU on the nic
 *
 * @param nic	the brick nic
 * @param mtu	MTU to be applied
 * @errp	set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_nic_set_mtu(struct pg_brick *brick, uint16_t mtu,
		   struct pg_error **errp);

/**
 * Get MTU of the nic
 *
 * @param nic	the brick nic
 * @mtu		set in case of success
 * @errp	set in case of an error
 * @return	0 on success, -1 on error
 */
int pg_nic_get_mtu(struct pg_brick *brick, uint16_t *mtu,
		   struct pg_error **errp);

/**
 * A structure used to retrieve statistics for an Ethernet port.
 * This is a copy from rte_eth_stats at V2.0.0.
 * TODO: handly copy fiel instead of memcpy.
 * this will assur us futur dpdk versions compatibility.
 */
struct pg_nic_stats {
	/**< Total number of successfully received packets. */
	uint64_t ipackets;
	/**< Total number of successfully transmitted packets.*/
	uint64_t opackets;
	/**< Total number of successfully received bytes. */
	uint64_t ibytes;
	/**< Total number of successfully transmitted bytes. */
	uint64_t obytes;
};

/**
 * get the stats of a nic
 *
 * @param nic	the brick nic
 * @param stats
 *   A pointer to a structure of type *nic_stats* to be filled with
 *   the values of device counters for the following set of statistics:
 *   - *ipackets* with the total of successfully received packets.
 *   - *opackets* with the total of successfully transmitted packets.
 *   - *ibytes*   with the total of successfully received bytes.
 *   - *obytes*   with the total of successfully transmitted bytes.
 *   - *ierrors*  with the total of erroneous received packets.
 *   - *oerrors*  with the total of failed transmitted packets.
 * @return:	nothig, this function should alway work
 */
void pg_nic_get_stats(struct pg_brick *nic,
		      struct pg_nic_stats *stats);

/** get the mac address of the nic brick
 *
 * @param nic	a pointer to a nic brick
 * @param addr	a pointer to a ether_addr structure to to filled with the
 *		Ethernet address
 */
void pg_nic_get_mac(struct pg_brick *nic, struct ether_addr *addr);

#endif  /* _PG_NIC_H */
