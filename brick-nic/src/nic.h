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

#ifndef _BRICKS_BRICK_NIC_INT_H_
#define _BRICKS_BRICK_NIC_H_

/* internal headers for nic brick */

/* dpdk driver init */
void devinitfn_pmd_igb_drv(void);
void devinitfn_rte_ixgbe_driver(void);
void devinitfn_pmd_pcap_drv(void);
void devinitfn_pmd_ring_drv(void);

extern struct rte_eth_dev rte_eth_devices[RTE_MAX_ETHPORTS];

#ifdef _BRICKS_BRICK_NIC_STUB_H_
extern uint16_t max_pkts;
#endif /* #ifdef _BRICKS_BRICK_NIC_STUB_H_ */

#endif /* _BRICKS_BRICK_NIC_H_*/
