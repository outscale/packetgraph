/* Copyright 2017 Outscale EURL
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

#ifndef _PG_UTILS_ERRORS_H
#define _PG_UTILS_ERRORS_H

#include <packetgraph/errors.h>

extern struct pg_error_context pg_error_global_ctx;

extern int pg_error_assert_enable;

#define PG_ERROR_FAIL_ASSERT						\
	"can use pg_assert only by calling pg_error_make_ctx before\n"

/**
 * Like g_assert, but use information store in pg_error_global_ctx
 * instead of current line, function and file.
 */
#define pg_assert(check) do {						\
		if (!pg_error_assert_enable) {				\
			fprintf(stderr, PG_ERROR_FAIL_ASSERT);		\
			abort();					\
		} else if (!(check)) {					\
			fprintf(stderr,					\
				"Error: file='%s' function='%s' line=%lu %s", \
				pg_error_global_ctx.file,		\
				pg_error_global_ctx.function,		\
				pg_error_global_ctx.line,		\
				"in sub function\n");			\
			fprintf(stderr,					\
				"%s'%s' function='%s' line=%u :%s(%s)'\n", \
				"At: file=", __FILE__, __func__, __LINE__, \
				"assertion fail: ", #check);		\
			abort();					\
		}							\
	} while (0)

struct pg_error_context *
pg_error_make_ctx_internal(const char *file, uint64_t line, const char *func);

#define pg_error_make_ctx(line_decalage)				\
	pg_error_make_ctx_internal(__FILE__, (__LINE__ + line_decalage), \
				   __func__);


#define pg_panic(args...) ({fprintf(stderr, args); abort(); })

#endif /* _PG_UTILS_ERRORS_H */
