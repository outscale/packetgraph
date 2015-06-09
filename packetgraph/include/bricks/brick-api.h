/* Copyright 2014 Nodalink EURL
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

#ifndef _BRICKS_BRICK_API_H_
#define _BRICKS_BRICK_API_H_

#include "utils/errors.h"
#include "utils/config.h"

struct brick;

enum side flip_side(enum side side);

/* testing */
int64_t brick_refcount(struct brick *brick);

/* relationship between bricks */
int brick_link(struct brick *west, struct brick *east,
	       struct switch_error **errp);

void brick_unlink(struct brick *brick, struct switch_error **errp);

/* polling */
int brick_poll(struct brick *brick,
		      uint16_t *count, struct switch_error **errp);

/* pkts count */
int64_t brick_pkts_count_get(struct brick *brick, enum side side);

/* constructors */
struct brick *nop_new(const char *name,
		      uint32_t west_max,
		      uint32_t east_max,
		      struct switch_error **errp);


struct brick *packetsgen_new(const char *name,
			uint32_t west_max,
			uint32_t east_max,
			enum side output,
			struct switch_error **errp);

struct brick *diode_new(const char *name,
			uint32_t west_max,
			uint32_t east_max,
			enum side output,
			struct switch_error **errp);

struct brick *vhost_new(const char *name, uint32_t west_max,
			uint32_t east_max, enum side output,
			struct switch_error **errp);

struct brick *switch_new(const char *name, uint32_t west_max,
			uint32_t east_max,
			struct switch_error **errp);

struct brick *hub_new(const char *name, uint32_t west_max,
			uint32_t east_max,
			struct switch_error **errp);

struct brick *nic_new(const char *name, uint32_t west_max,
		      uint32_t east_max,
		      enum side output,
		      const char *ifname,
		      struct switch_error **errp);

struct brick *nic_new_by_id(const char *name, uint32_t west_max,
			    uint32_t east_max,
			    enum side output,
			    uint8_t portid,
			    struct switch_error **errp);


/* NIC functions */

/**
 * A structure used to retrieve statistics for an Ethernet port.
 * Copied from rte_eth_stats
 */
struct nic_stats {
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

void nic_get_stats(struct brick *nic,
		   struct nic_stats *stats);

/* destructor */
void brick_destroy(struct brick *brick);

#endif
