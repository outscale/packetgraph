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

#ifndef _PG_NIC_INT_H
#define _PG_NIC_INT_H

/* internal headers for nic brick */

extern struct rte_eth_dev rte_eth_devices[RTE_MAX_ETHPORTS];

#ifdef PG_NIC_STUB
extern uint16_t max_pkts;
#endif /* PG_NIC_STUB */

#endif /* _PG_NIC_INT_H */
