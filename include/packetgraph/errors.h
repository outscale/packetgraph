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

#ifndef _PG_ERRORS_H
#define _PG_ERRORS_H

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

/* To use the error infrastructure pass a struct pg_error **errp with
 * *errp == NULL to the various functions.
 * Then when an error occurs the functions will do *errp = error_new_errno() or
 * *errp = error_new() and the caller will be able to check for *errp != NULL.
 * After consumption of the error free it with error_free().
 */

struct pg_error_context {
	const char *file;
	int has_line;
	uint64_t line;
	const char *function;
};


struct pg_error {
	struct pg_error_context context;
	char *message;
	int has_err_no;
	int32_t err_no;
};


/* NOTE: Do not pass an *errp != NULL to functions using struct pg_error. */
#define pg_error_new_ctx_errno(ctx, err_no, ...) __pg_error_new(err_no,	\
								ctx.file, \
								ctx.line, \
								ctx.function, \
								__VA_ARGS__)

#define pg_error_new_ctx(ctx, ...) pg_error_new_ctx_errno(0,		\
							  ctx,		\
							  __VA_ARGS__)


#define pg_error_new_errno(err_no, ...) __pg_error_new(err_no,		\
						       __FILE__,	\
						       __LINE__,	\
						       __func__,	\
						       __VA_ARGS__)

#define pg_error_new(...) pg_error_new_errno(0, __VA_ARGS__)

#define PG_SUB_ERROR_FMT			\
	"\n\t-- Error: file='%s' function='%s' line=%lu\n\t   %s"

#define pg_error_prepend(error, format, ...)				\
	pg_error_prepend_(error, __FILE__,  __LINE__, __func__,		\
			  format PG_SUB_ERROR_FMT, __VA_ARGS__)

#define pg_error_prepend_(error, file_, line_, function_, format, ...)	\
	do {								\
		char *file_tmp = (char *)(error)->context.file;		\
		char *func_tmp = (char *)(error)->context.function;	\
		char *msg_tmp = (error)->message;			\
		(error)->context.file = g_strdup(file_);		\
		(error)->context.has_line = 1;				\
		(error)->context.line = line_;				\
		(error)->context.function = g_strdup(function_);	\
		(error)->message = g_strdup_printf(format, __VA_ARGS__,	\
						   file_tmp, func_tmp,	\
						   (error)->context.line, \
						   msg_tmp);		\
		g_free(file_tmp);					\
		g_free(func_tmp);					\
		g_free(msg_tmp);					\
	} while (0)


PG_WARN_UNUSED
struct pg_error *__pg_error_new(int err_no, const char *file,
				uint64_t line, const char *function,
				const char *format, ...)

				__attribute__((__format__(__printf__, 5, 6)));

void pg_error_free(struct pg_error *error);

void pg_error_print(struct pg_error *error);

bool pg_error_is_set(struct pg_error **error);

#endif /* _PG_ERRORS_H */
