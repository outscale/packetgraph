/* Copyright 2015 Outscale SAS
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

#ifndef _PG_NIC_H
#define _PG_NIC_H

#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/**
 * Create a new nic brick
 * Note: this brick support pg_brick_rx_bytes and pg_brick_tx_bytes.
 *
 * @param   name the brick's name
 * @param   ifname the name of the interface you want to use.
 *          This is the same syntax as --vdev in DPDK:
 *          http://pktgen.readthedocs.org/en/latest/usage_eal.html?highlight=vdev
 *          For example, if you want to open a system NIC, put:
 *          "eth_pcap0,iface=eth2"
 * @param   errp set in case of an error
 * @return  a pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_nic_new(const char *name,
			    const char *ifname,
			    struct pg_error **errp);

/**
 * Create a new nic brick
 *
 * @param   name name of the brick
 * @param   portid the id of the interface you want to use
 * @param   errp set in case of an error
 * @return  a pointer to a brick structure, on success, 0 on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_nic_new_by_id(const char *name,
				  uint16_t portid,
				  struct pg_error **errp);

/**
 * Get number of available DPDK ports
 *
 * @return  number of DPDK ports
 */
int pg_nic_port_count(void);

/**
 * Set a custom MTU on the nic
 *
 * @param   brick the brick nic
 * @param   mtu MTU to be applied
 * @param   errp set in case of an error
 * @return  0 on success, -1 on error
 */
int pg_nic_set_mtu(struct pg_brick *brick, uint16_t mtu,
		   struct pg_error **errp);

/**
 * Get MTU of the nic
 *
 * @param   brick the nic brick
 * @param   mtu set in case of success
 * @param   errp set in case of an error
 * @return  0 on success, -1 on error
 */
int pg_nic_get_mtu(struct pg_brick *brick, uint16_t *mtu,
		   struct pg_error **errp);

/**
 * Get the mac address of the nic brick
 *
 * @param   nic a pointer to a nic brick
 * @param   addr a pointer to a ether_addr structure to to filled with the
 *          Ethernet address
 */
void pg_nic_get_mac(struct pg_brick *nic, struct ether_addr *addr);

#define PG_NIC_RX_OFFLOAD_VLAN_STRIP  0x00000001
#define PG_NIC_RX_OFFLOAD_IPV4_CKSUM  0x00000002
#define PG_NIC_RX_OFFLOAD_UDP_CKSUM   0x00000004
#define PG_NIC_RX_OFFLOAD_TCP_CKSUM   0x00000008
#define PG_NIC_RX_OFFLOAD_TCP_LRO     0x00000010
#define PG_NIC_RX_OFFLOAD_QINQ_STRIP  0x00000020
#define PG_NIC_RX_OFFLOAD_OUTER_IPV4_CKSUM 0x00000040

#define PG_NIC_TX_OFFLOAD_VLAN_INSERT 0x00000001
#define PG_NIC_TX_OFFLOAD_IPV4_CKSUM  0x00000002
#define PG_NIC_TX_OFFLOAD_UDP_CKSUM   0x00000004
#define PG_NIC_TX_OFFLOAD_TCP_CKSUM   0x00000008
#define PG_NIC_TX_OFFLOAD_SCTP_CKSUM  0x00000010
#define PG_NIC_TX_OFFLOAD_TCP_TSO     0x00000020
#define PG_NIC_TX_OFFLOAD_UDP_TSO     0x00000040
#define PG_NIC_TX_OFFLOAD_OUTER_IPV4_CKSUM 0x00000080
#define PG_NIC_TX_OFFLOAD_QINQ_INSERT 0x00000100

/**
 * Get capabilities of the nic
 *
 * @param   nic pointer to a nic brick
 * @param   rx capabilities, see PG_NIC_RX_* flags
 * @param   tx capabilities, see PG_NIC_TX_* flags
 */
void pg_nic_capabilities(struct pg_brick *nic, uint32_t *rx, uint32_t *tx);

#endif  /* _PG_NIC_H */
