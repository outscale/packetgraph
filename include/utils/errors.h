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

#ifndef _UTILS_ERRORS_H
#define _UTILS_ERRORS_H

/**
 * These define are made to keep using the Linux coding style.
 * They point to structures in the protocol buffer header.
 */
#include "error.pb-c.h"
#define error_context	_ErrorContext
#define switch_error	_SwitchError

/* To use the error infrastructure pass a struct switch_error **errp with
 * *errp == NULL to the various functions.
 * Then when an error occurs the functions will do *errp = error_new_errno() or
 * *errp = error_new() and the caller will be able to check for *errp != NULL.
 * After consumption of the error free it with error_free().
 */

/* NOTE: Do not pass an *errp != NULL to functions using struct switch_error. */

#define error_new_errno(err_no, ...) __error_new(err_no,	\
						      __FILE__,	\
						      __LINE__,	\
						      __func__, \
						       __VA_ARGS__)

#define error_new(...) error_new_errno(0, __VA_ARGS__)

struct switch_error *__error_new(int err_no, const char *file, uint64_t line,
				 const char *function, const char *format, ...);

void error_free(struct switch_error *error);

void error_print(struct switch_error *error);

int error_is_set(struct switch_error **error);

#endif
