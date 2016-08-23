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
	char *file;
	int has_line;
	uint64_t line;
	char *function;
};


struct pg_error {
	struct pg_error_context context;
	char *message;
	int has_err_no;
	int32_t err_no;
};

/* NOTE: Do not pass an *errp != NULL to functions using struct pg_error. */

#define pg_error_new_errno(err_no, ...) __pg_error_new(err_no,		\
						       __FILE__,	\
						       __LINE__,	\
						       __func__,	\
						       __VA_ARGS__)

#define pg_error_new(...) pg_error_new_errno(0, __VA_ARGS__)

PG_WARN_UNUSED
struct pg_error *__pg_error_new(int err_no, const char *file,
				uint64_t line, const char *function,
				const char *format, ...);

void pg_error_free(struct pg_error *error);

void pg_error_print(struct pg_error *error);

bool pg_error_is_set(struct pg_error **error);

#endif /* _PG_ERRORS_H */
