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

#ifndef _PG_PRINT_PRINTER_H
#define _PG_PRINT_PRINTER_H

#include <glib.h>

/**
 * Inspect a packet and write a short summary
 *
 * @param	data packet's data.
 * @param	size packet's size.
 * @param	output where to write informations
 */
void print_summary(void *data, size_t size, FILE *ouput);

/**
 * Inspect a packet and write it's raw data (hex)
 *
 * @param	data packet's data.
 * @param	size packet's size.
 * @param	output where to write informations
 */
void print_raw(void *data, size_t size, FILE *output);

#endif  /* _PG_PRINT_PRINTER_H */
