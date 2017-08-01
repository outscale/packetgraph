/* Copyright 2016 Outscale SAS
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
#ifndef _PG_RXTX_H
#define _PG_RXTX_H

/**
 * The rx callback is called each time packets flow to brick.
 * To send a response, check tx callback.
 *
 * @param   brick pointer to the rxtx brick
 * @param   rx_burst array of received packets.
 *          You MUST NOT write or free data in this array.
 * @param   rx_burst_len array's size of received packets
 * @param   private_data give back user's data
 */
typedef void (*pg_rxtx_rx_callback_t)(struct pg_brick *brick,
				      pg_packet_t **rx_burst,
				      uint16_t rx_burst_len,
				      void *private_data);

/**
 * The tx callback is called each time the brick is polled.
 *
 * @param   brick pointer to the rxtx brick
 * @param   tx_burst array of pre-allocated packets.
 *          The array size if given with tx_burst_len.
 *          Use pg_packet_data to get a pointer where to write data.
 *          Use pg_packet_set_len to set len of your packet.
 *          You must not write data once callback is done.
 *          You will get back the same packet array at each	call.
 *          You can take advantage of this to only write what you need to.
 * @param   rx_burst_len number of packets ready to be sent.
 *          Must not be > PG_RXTX_MAX_TX_BURST_LEN
 * @param   private_data give back user's data
 */
typedef void (*pg_rxtx_tx_callback_t)(struct pg_brick *brick,
				      pg_packet_t **tx_burst,
				      uint16_t *tx_burst_len,
				      void *private_data);

/* maximal burst lenght when sending packets. */
#define PG_RXTX_MAX_TX_BURST_LEN 64

/**
 * Create a new rxtx brick (monopole).
 *
 * An rxtx brick allow users to easily create their own packet processing
 * applications by just providing callbacks to send and receive packets.
 *
 * @param   name name of the brick
 * @param   rx optional callback used when packets flow to brick
 * @param   tx optional callback used to send packets to graph
 * @param   private_data pointer to user's data
 * @return  a pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_rxtx_new(const char *name,
			     pg_rxtx_rx_callback_t rx,
			     pg_rxtx_tx_callback_t tx,
			     void *private_data);

/**
 * Get back private data if needed outside rx or tx.
 * @param   brick pointer to the rxtx brick
 * @return  pointer to provided private data during pg_rxtx_new.
 */
void *pg_rxtx_private_data(struct pg_brick *brick);

#endif  /* _PG_RXTX_H */
