/* Copyright 2014 Nodalink EURL
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

#include <glib.h>
#include <stdarg.h>
#include <stdio.h>

#include "utils/errors.h"

int pg_error_assert_enable;
struct pg_error_context pg_error_global_ctx;

struct pg_error_context *
pg_error_make_ctx_internal(const char *file, uint64_t line, const char *func)
{
	pg_error_global_ctx.file = file;
	pg_error_global_ctx.has_line = 1;
	pg_error_global_ctx.line = line;
	pg_error_global_ctx.function = func;
	pg_error_assert_enable = 1;
	return &pg_error_global_ctx;
}

/**
 * Build a new error protocol buffer object
 *
 * @param	err_no if not null the errno to set in the error object
 * @param	file the name of the file where the error is happening
 * @param	line the line of the file where the error is happening
 * @param	function the function where the error is happening
 * @param	format the message format
 * @return	the newly allocated struct pg_error
 */
struct pg_error *__pg_error_new(int err_no, const char *file, uint64_t line,
				const char *function, const char *format, ...)
{
	va_list args;
	struct pg_error *error = g_new0(struct pg_error, 1);

	error->context.file = g_strdup(file);
	error->context.has_line = 1;
	error->context.line = line;
	error->context.function = g_strdup(function);

	error->has_err_no = err_no;
	error->err_no = err_no;

	va_start(args, format);
	error->message = g_strdup_vprintf(format, args);
	va_end(args);

	return error;
}

/**
 * Free an error
 *
 * @param	error the error to free
 */
void pg_error_free(struct pg_error *error)
{
	if (!error)
		return;
	g_free((char *)error->context.file);
	g_free((char *)error->context.function);

	g_free(error->message);
	g_free(error);
}

/**
 * Print an error to stderr
 *
 * @param	the error to print
 */
void pg_error_print(struct pg_error *error)
{
	if (!error) {
		fprintf(stderr, "Error is NULL\n");
		return;
	}

	fprintf(stderr,
		"Error:  file='%s' function='%s' line=%lu errno=%i:\n\t%s",
		error->context.file, error->context.function,
		error->context.line, error->err_no,
		error->message);
}

/**
 * Check if *errp is not null
 *
 * Do not make use of this functions is performance critical path.
 *
 * @param	errp an error pointer
 * @return	1 if the error is set else 0
 */
bool pg_error_is_set(struct pg_error **errp)
{
	return !!*errp;
}
