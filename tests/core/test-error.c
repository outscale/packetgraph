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

#include <errno.h>
#include <glib.h>

#include <packetgraph/errors.h>
#include "tests.h"

static void test_error_lifecycle(void)
{
	struct pg_error *error = NULL;

	error = pg_error_new("Percolator overflow");
	pg_error_free(error);

	error = pg_error_new_errno(EIO, "Data lost");
	pg_error_free(error);
}

static void test_error_vanilla(void)
{
	uint64_t line;
	struct pg_error *error = NULL;

	line = __LINE__ + 1;
	error = pg_error_new("Trashcan overflow");

	g_assert(error->message);
	g_assert_cmpstr(error->message, ==,
			"Trashcan overflow");

	g_assert(!error->has_err_no);

	g_assert(error->context.file);
	g_assert_cmpstr(g_path_get_basename(error->context.file),
			==,
			"test-error.c");

	g_assert(error->context.function);
	g_assert_cmpstr(error->context.function,
			==,
			"test_error_vanilla");

	g_assert(error->context.has_line);
	g_assert(error->context.line == line);

	pg_error_free(error);
}

static void test_error_errno(void)
{
	uint64_t line;
	struct pg_error *error = NULL;

	line = __LINE__ + 1;
	error = pg_error_new_errno(EIO, "Bad write");

	g_assert(error->message);
	g_assert_cmpstr(error->message, ==,
			"Bad write");

	g_assert(error->has_err_no);
	g_assert(error->err_no == EIO);

	g_assert(error->context.file);
	g_assert_cmpstr(g_path_get_basename(error->context.file),
			==,
			"test-error.c");

	g_assert(error->context.function);
	g_assert_cmpstr(error->context.function,
			==,
			"test_error_errno");

	g_assert(error->context.has_line);
	g_assert(error->context.line == line);

	pg_error_free(error);
}

static void test_error_format(void)
{
	struct pg_error *error = NULL;

	error = pg_error_new_errno(EIO, "Bad write file=%s sector=%i", "foo", 5);
	g_assert(error->message);
	g_assert_cmpstr(error->message, ==, "Bad write file=foo sector=5");
	pg_error_free(error);

	error = pg_error_new("Bad write file=%s sector=%i", "bar", 10);
	g_assert(error->message);
	g_assert_cmpstr(error->message, ==, "Bad write file=bar sector=10");
	pg_error_free(error);
}

void test_error(void)
{
	g_test_add_func("/error/lifecycle", test_error_lifecycle);
	g_test_add_func("/error/vanilla", test_error_vanilla);
	g_test_add_func("/error/errno", test_error_errno);
	g_test_add_func("/error/format", test_error_format);
}
