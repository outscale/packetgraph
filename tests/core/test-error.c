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

#include <stdio.h>
#include <memory.h>
#include <unistd.h>

#include <errno.h>
#include <glib.h>

#include "utils/errors.h"
#include "utils/tests.h"
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
	char *pathname;

	line = __LINE__ + 1;
	error = pg_error_new("Trashcan overflow");

	g_assert(error->message);
	g_assert_cmpstr(error->message, ==,
			"Trashcan overflow");

	g_assert(!error->has_err_no);

	g_assert(error->context.file);
	pathname = g_path_get_basename(error->context.file);
	g_assert_cmpstr(pathname, ==, "test-error.c");
	g_free(pathname);

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
	char *pathname;

	line = __LINE__ + 1;
	error = pg_error_new_errno(EIO, "Bad write");

	g_assert(error->message);
	g_assert_cmpstr(error->message, ==,
			"Bad write");

	g_assert(error->has_err_no);
	g_assert(error->err_no == EIO);

	g_assert(error->context.file);
	pathname = g_path_get_basename(error->context.file);
	g_assert_cmpstr(pathname, ==, "test-error.c");
	g_free(pathname);

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

	error = pg_error_new_errno(EIO, "Bad write file=%s sector=%i",
				   "foo", 5);
	g_assert(error->message);
	g_assert_cmpstr(error->message, ==, "Bad write file=foo sector=5");
	pg_error_free(error);

	error = pg_error_new("Bad write file=%s sector=%i", "bar", 10);
	g_assert(error->message);
	g_assert_cmpstr(error->message, ==, "Bad write file=bar sector=10");
	pg_error_free(error);
}

static void test_error_make_ctx_internal(void)
{
	struct pg_error_context *
	ec = pg_error_make_ctx(0);
	unsigned int line_or_error = __LINE__ - 1;

	g_assert(ec->has_line == 1);
	g_assert(!strcmp(ec->file, "tests/core/test-error.c"));
	g_assert(ec->line == line_or_error);
	g_assert(!strcmp(ec->function, "test_error_make_ctx_internal"));
}
static void test_error_print(void)
{
	/* This buffer will store the expected message. */
	char expected_output[240];

	/* Create an error for testing. */
	struct pg_error *
	err = pg_error_new(0);
	int line_or_error = __LINE__ - 1;

	/* Prepare the redirection of stderr. */
	char buffer[BUFSIZ] = {0};
	int out_pipe[2];
	int saved_stderr;

	/* Save current output to recover later. */
	saved_stderr = dup(STDERR_FILENO);
	/* Redirect stderr to buffer. */
	pipe(out_pipe);
	dup2(out_pipe[1], STDERR_FILENO);
	close(out_pipe[1]);

	/* Test pg_error_print. */
	pg_error_print(err);

	/* Read the output of pg_error_print. */
	fflush(stdout);
	read(out_pipe[0], buffer, BUFSIZ);
	/* Check the result. */
	sprintf(expected_output, "Error:  file='tests/core/test-error.c'"
		   " function='%s'"
		   " line=%d errno=0:\n\t(null)", __func__, line_or_error);
	g_assert(!strcmp(expected_output, buffer));
	/* Free the error. */
	pg_error_free(err);
	/* Clear the buffer */
	memset (buffer, 0, BUFSIZ);

	/* Test pg_error_print with NULL. */
	pg_error_print(NULL);

	/* Read the output of pg_error_print. */
	fflush(stdout);
	read(out_pipe[0], buffer, BUFSIZ);
	/* Check the result. */
	g_assert(!strcmp("Error is NULL\n", buffer));
	pg_error_free(NULL);
	/* Redirect stderr to the terminal. */
	dup2(saved_stderr, STDERR_FILENO);
	close(saved_stderr);
}

void test_error(void)
{
	pg_test_add_func("/core/error/lifecycle", test_error_lifecycle);
	pg_test_add_func("/core/error/vanilla", test_error_vanilla);
	pg_test_add_func("/core/error/errno", test_error_errno);
	pg_test_add_func("/core/error/format", test_error_format);
	pg_test_add_func("/core/error/error_make_ctx_internal",
			 test_error_make_ctx_internal);
	pg_test_add_func("/core/error/error_print", test_error_print);
}
