/* Copyright 2016 Outscale SAS
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
#ifndef _PG_RXTX_H
#define _PG_RXTX_H

struct pg_rxtx_packet {
	uint8_t *data;
	uint16_t *len;
};

/**
 * The rx callback is called each time packets flow to brick.
 * To send a response, check tx callback.
 *
 * @param	brick pointer to the rxtx brick
 * @param	rx_burst array of received packets.
 *		You MUST NOT write or free data in this array.
 * @param	rx_burst_len array's size of received packets
 * @param	private_data give back user's data
 */
typedef void (*pg_rxtx_rx_callback_t)(struct pg_brick *brick,
				      const struct pg_rxtx_packet *rx_burst,
				      uint16_t rx_burst_len,
				      void *private_data);

/**
 * The tx callback is called each time the brick is polled.
 *
 * @param	brick pointer to the rxtx brick
 * @param	tx_burst array of pre-allocated packets.
 *		You can write packets here but once the callback end, you
 *		should not touch packet anymore.
 *		See PG_RXTX_MAX_TX_BURST_LEN and PG_RXTX_MAX_TX_PACKET_LEN
 *		Note that that you will get back the same burst pointer at each
 *		burst so you can get advantage of this to only write what you
 *		need.
 * @param	private_data give back user's data
 * @param	rx_burst_len number of packets ready to be sent
 */
typedef void (*pg_rxtx_tx_callback_t)(struct pg_brick *brick,
				      struct pg_rxtx_packet *tx_burst,
				      uint16_t *tx_burst_len,
				      void *private_data);

/* maximal burst lenght when sending packets. */
#define PG_RXTX_MAX_TX_BURST_LEN 64
/* maximan packet lenght user can write. */
#define PG_RXTX_MAX_TX_PACKET_LEN 1500

/**
 * Create a new rxtx brick (monopole).
 *
 * An rxtx brick allow users to easily create their own packet processing
 * applications by just providing callbacks to send and receive packets.
 *
 * @param	name name of the brick
 * @param	rx optional callback used when packets flow to brick
 * @param	tx optional callback used to send packets to graph
 * @param	private_data pointer to user's data
 * @param	errp is set in case of an error
 * @return	a pointer to a brick structure on success, NULL on error
 */
struct pg_brick *pg_rxtx_new(const char *name,
			     pg_rxtx_rx_callback_t rx,
			     pg_rxtx_tx_callback_t tx,
			     void *private_data);

/**
 * Get back private data if needed outside rx or tx.
 * @param	brick pointer to the rxtx brick
 * @return	pointer to provided private data during pg_rxtx_new.
 */
void *pg_rxtx_private_data(struct pg_brick *brick);

#endif  /* _PG_RXTX_H */
