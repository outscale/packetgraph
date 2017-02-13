/* Copyright 2017 Outscale SAS
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

#include "utils/bitmask.h"
#include "utils/tests.h"
#include "tests.h"
#include <stdio.h>

static void test_bitmask_int(void)
{
	g_assert(pg_mask_firsts(0) == 0);
	g_assert(pg_mask_firsts(3) == 7);
	g_assert(pg_mask_firsts(64) == 0xffffffffffffffff);
	g_assert(pg_mask_firsts(63) == 0x7fffffffffffffff);
}

void test_bitmask(void)
{
	pg_test_add_func("/core/bitmask", test_bitmask_int);
}
