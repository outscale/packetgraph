/* Copyright 2014 Nodalink EURL
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

#ifndef _PG_LIFECYCLE_H
#define _PG_LIFECYCLE_H

void mp_hdlr_init_ops_stack(void);

/**
 * Initialize packetgraph.
 * This function should be called before any other packetgraph function.
 *
 * @param	size of argv
 * @param	argv all arguments passwed to packetgraph, it may contain
 *		DPDK arugments.
 * @param	errp is set in case of an error
 * @return	return where the program's parameters count starts
 */
int pg_start(int argc, char **argv, struct pg_error **errp);

/**
 * Initilize packetgraph.
 * This function should be called before any other packetgraph function.
 *
 * @param       dpdk_args a string with all dpdk arguments
 * @return      -1 in case of error, 0 otherwise
 */
int pg_start_str(const char *dpdk_args);

/**
 * Uninitialize packetgraph.
 * This function should be called when the application exits.
 */
void pg_stop(void);

#endif /* _PG_LIFECYCLE_H */
