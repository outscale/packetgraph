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

#ifndef _PG_PRINT_H
#define _PG_PRINT_H

#include <stdbool.h>
#include <packetgraph/common.h>
#include <packetgraph/errors.h>

/* Print flags. */
enum pg_print_flags {
	/* Print a summary of the packet (size, protocols, etc..). */
	PG_PRINT_FLAG_SUMMARY = 1,
	/* Show timestamp when printing. */
	PG_PRINT_FLAG_TIMESTAMP = 2,
	/* Show packet size when printing. */
	PG_PRINT_FLAG_SIZE = 4,
	/* Print brick name and direction. */
	PG_PRINT_FLAG_BRICK = 8,
	/* Print raw packet data. */
	PG_PRINT_FLAG_RAW = 16,
	/* Print in pcap format, imply PG_PRINT_FLAG_CLOSE_FILE */
	PG_PRINT_FLAG_PCAP = 32,
	/* close output */
	PG_PRINT_FLAG_CLOSE_FILE = 64,
};

#define PG_PRINT_FLAG_MAX (PG_PRINT_FLAG_SUMMARY | PG_PRINT_FLAG_TIMESTAMP | \
			   PG_PRINT_FLAG_SIZE | PG_PRINT_FLAG_BRICK | \
			   PG_PRINT_FLAG_RAW)

/**
 * Create a new print brick
 *
 * @param	name name of the brick
 * @param	west_max maximum of links you can connect on the west side
 * @param	east_max maximum of links you can connect on the east side
 * @param	output file descriptor where to write packets informations
 *		NULL means to use the standard output (stdout).
 * @param	flags print flags from enum pg_print_flags.
 * @param	type_filter ethernet type skiped at printing,
 *		NULL to skip none.
 * @param	errp is set in case of an error
 * @return	a pointer to a brick structure on success, NULL on error
 */
struct pg_brick *pg_print_new(const char *name,
			      FILE *output,
			      int flags,
			      uint16_t *type_filter,
			      struct pg_error **errp);

/**
 * Set print flags of a print brick.
 *
 * @param	brick pointer to a print brick
 * @param	flags print flags from enum pg_print_flags.
 */
void pg_print_set_flags(struct pg_brick *brick, int flags);

#endif  /* _PG_PRINT_H */
