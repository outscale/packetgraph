/* Copyright 2017 Outscale SAS
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
#ifndef _PG_USER_DIPOLE_H
#define _PG_USER_DIPOLE_H

/**
 * @param   brick the brick
 * @param   output side on which the packets will be send
 * @param   pkts the packet
 * @param   pkts_mask represent inputs and output packets,if you want
 *          to block a packet, just remove it's index from pkts_mask
 *          Example:
 *          a pkts_mast equal to 0x6 mean that pkts[0] contain no valide
 *          packet. But pkts[1] and pkts[2] can be safely acess and modifie
 *          if then you set pkts_mast to 0x4, it mean that pkts[1] must be
 *          free, and must be block by this brick.
 * @param   private_data what has been set at pg_user_dipole_new
 */
typedef void (*pg_user_dipole_callback_t)(struct pg_brick *brick,
					  enum pg_side output,
					  pg_packet_t **pkts,
					  uint64_t *pkts_mask,
					  void *private_data);

/**
 * Create a new user_dipole brick (monopole).
 *
 * An user_dipole brick allow users to easily create their own packet processing
 * applications by just providing callbacks to send and receive packets.
 *
 * @param   name name of the brick
 * @param   callback to send and receive packets
 * @param   private_data pointer to user's data
 * @param   errp is set in case of an error
 * @return  a pointer to a brick structure on success, NULL on error
 */
PG_WARN_UNUSED
struct pg_brick *pg_user_dipole_new(const char *name,
				    pg_user_dipole_callback_t callback,
				    void *private_data,
				    struct pg_error **errp);

/**
 * Get back private data if needed outside rx or tx.
 * @param   brick pointer to the user_dipole brick
 * @return  pointer to provided private data during pg_user_dipole_new.
 */
void *pg_user_dipole_private_data(struct pg_brick *brick);


#endif  /* _PG_USER_DIPOLE_H */
