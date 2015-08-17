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

#ifndef _BRICKS_BRICK_NIC_H_
#define _BRICKS_BRICK_NIC_H_

#include <packetgraph/common.h>
#include <packetgraph/utils/errors.h>

#ifndef __cplusplus
#include <rte_ether.h>
#else
struct ether_addr;
#endif

/**
 * Start brick-nic library, must be call before any usage of this lib
 */
void pg_nic_start(void);

/**
 * Create a new nic brick
 *
 * @name:	name of the brick
 * @west_max:	maximum of links you can connect on the west side
 * @east_max:	maximum of links you can connect on the east side
 * @ifname:	the name of the interface you want to use(example: "eth0")
 * @errp:	set in case of an error
 * @return:	a pointer to a brick structure, on success, 0 on error
 */
struct pg_brick *pg_nic_new(const char *name, uint32_t west_max,
			 uint32_t east_max,
			 enum pg_side output,
			 const char *ifname,
			 struct pg_error **errp);

/**
 * Create a new nic brick
 *
 * @name:	name of the brick
 * @west_max:	maximum of links you can connect on the west side
 * @east_max:	maximum of links you can connect on the east side
 * @output:	The side of the output (so the side of the nic)
 * @portid:	the id of the interface you want to use
 * @errp:	set in case of an error
 * @return:	a pointer to a brick structure, on success, 0 on error
 */
struct pg_brick *pg_nic_new_by_id(const char *name, uint32_t west_max,
				  uint32_t east_max,
				  enum pg_side output,
				  uint8_t portid,
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
	/**< Total of RX missed packets (e.g full FIFO). */
	uint64_t imissed;
	/**< Total of RX packets with CRC error. */
	uint64_t ibadcrc;
	/**< Total of RX packets with bad length. */
	uint64_t ibadlen;
	/**< Total number of erroneous received packets. */
	uint64_t ierrors;
	/**< Total number of failed transmitted packets. */
	uint64_t oerrors;
	/**< Total number of multicast received packets. */
	uint64_t imcasts;
	/**< Total number of RX mbuf allocation failures. */
	uint64_t rx_nombuf;
	/**< Total number of RX packets matching a filter. */
	uint64_t fdirmatch;
	/**< Total number of RX packets not matching any filter. */
	uint64_t fdirmiss;
	/**< Total nb. of XON pause frame sent. */
	uint64_t tx_pause_xon;
	/**< Total nb. of XON pause frame received. */
	uint64_t rx_pause_xon;
	/**< Total nb. of XOFF pause frame sent. */
	uint64_t tx_pause_xoff;
	/**< Total nb. of XOFF pause frame received. */
	uint64_t rx_pause_xoff;
	/**< Total number of queue RX packets. */
	uint64_t q_ipackets[RTE_ETHDEV_QUEUE_STAT_CNTRS];
	/**< Total number of queue TX packets. */
	uint64_t q_opackets[RTE_ETHDEV_QUEUE_STAT_CNTRS];
	/**< Total number of successfully received queue bytes. */
	uint64_t q_ibytes[RTE_ETHDEV_QUEUE_STAT_CNTRS];
	/**< Total number of successfully transmitted queue bytes. */
	uint64_t q_obytes[RTE_ETHDEV_QUEUE_STAT_CNTRS];
	/**< Total number of queue packets received that are dropped. */
	uint64_t q_errors[RTE_ETHDEV_QUEUE_STAT_CNTRS];
	/**< Total number of good packets received from loopback,VF Only */
	uint64_t ilbpackets;
	/**< Total number of good packets transmitted to loopback,VF Only */
	uint64_t olbpackets;
	/**< Total number of good bytes received from loopback,VF Only */
	uint64_t ilbbytes;
	/**< Total number of good bytes transmitted to loopback,VF Only */
	uint64_t olbbytes;
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

#endif  /* _BRICKS_BRICK_NIC_H_ */
