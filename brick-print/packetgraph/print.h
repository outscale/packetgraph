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

#ifndef _BRICKS_BRICK_PRINT_H_
#define _BRICKS_BRICK_PRINT_H_

#include <packetgraph/common.h>
#include <packetgraph/utils/errors.h>

/* Print flags. */
enum print_flags {
	/* Print a summary of the packet (size, protocols, etc..). */
	PRINT_FLAG_SUMMARY = 1,
	/* Show timestamp when printing. */
	PRINT_FLAG_TIMESTAMP = 2,
	/* Show packet size when printing. */
	PRINT_FLAG_SIZE = 4,
	/* Print brick name and direction. */
	PRINT_FLAG_BRICK = 8,
	/* Print raw packet data. */
	PRINT_FLAG_RAW = 16,
};

#define PRINT_FLAG_MAX (PRINT_FLAG_SUMMARY | PRINT_FLAG_TIMESTAMP | \
			PRINT_FLAG_SIZE | PRINT_FLAG_BRICK | \
			PRINT_FLAG_RAW)

/**
 * Create a new print brick
 *
 * @param	name name of the brick
 * @param	west_max maximum of links you can connect on the west side
 * @param	east_max maximum of links you can connect on the east side
 * @param	output file descriptor where to write packets informations
 *		NULL means to use the standard output (stdout).
 * @param	flags print flags from enum print_flags.
 * @param	type_filter ethernet type skiped at printing.
 * @param	errp is set in case of an error
 * @return	a pointer to a brick structure, on success, 0 on error
 */
struct brick *print_new(const char *name,
			uint32_t west_max,
			uint32_t east_max,
			FILE *output,
			int flags,
			uint16_t *type_filter,
			struct switch_error **errp);

/**
 * Set print flags of a print brick.
 *
 * @param	brick pointer to a print brick
 * @param	flags print flags from enum print_flags.
 */
void print_set_flags(struct brick *brick, int flags);

#endif  /* _BRICKS_BRICK_PRINT_H_ */
