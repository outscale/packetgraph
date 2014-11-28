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

#include "bricks/brick.h"

static void test_brick_core_simple_lifecycle(void)
{
	struct brick *brick;

	brick = brick_new("foo");
	g_assert(!brick);

	brick = brick_new("nop");
	g_assert(brick);

	brick_decref(brick);

	brick = brick_decref(NULL);
	g_assert(!brick);
}

static void test_brick_core_refcount(void)
{
	struct brick *brick;

	brick = brick_new("nop");
	g_assert(brick);

	/* Use the brick twice */
	brick_incref(brick);
	brick_incref(brick);

	/* Release it twice */
	brick = brick_decref(brick);
	g_assert(brick);
	brick = brick_decref(brick);
	g_assert(brick);

	/* finally destroy the brick */
	brick = brick_decref(brick);
	g_assert(!brick);
}

static void test_brick_core_link(void)
{
	struct brick *west_brick, *middle_brick, *east_brick;
	int64_t refcount;
	int ret;

	west_brick = brick_new("nop");
	g_assert(west_brick);

	middle_brick = brick_new("nop");
	g_assert(middle_brick);

	east_brick = brick_new("nop");
	g_assert(east_brick);

	ret = brick_west_link(middle_brick, west_brick);
	g_assert(ret);

	ret = brick_east_link(middle_brick, east_brick);
	g_assert(ret);

	refcount = brick_refcount(west_brick);
	g_assert(refcount == 2);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 3);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 2);

	brick_unlink(west_brick);
	refcount = brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 2);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 2);

	brick_unlink(east_brick);
	refcount = brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 1);

	/* destroy */
	brick_decref(west_brick);
	brick_decref(middle_brick);
	brick_decref(east_brick);
}

static void test_brick_core_multiple_link(void)
{
	struct brick *west_brick, *middle_brick, *east_brick;
	int64_t refcount;
	int ret;

	west_brick = brick_new("nop");
	g_assert(west_brick);

	middle_brick = brick_new("nop");
	g_assert(middle_brick);

	east_brick = brick_new("nop");
	g_assert(east_brick);

	ret = brick_west_link(middle_brick, west_brick);
	g_assert(ret);
	ret = brick_west_link(middle_brick, west_brick);
	g_assert(ret);

	ret = brick_east_link(middle_brick, east_brick);
	g_assert(ret);
	ret = brick_east_link(middle_brick, east_brick);
	g_assert(ret);
	ret = brick_east_link(middle_brick, east_brick);
	g_assert(ret);

	refcount = brick_refcount(west_brick);
	g_assert(refcount == 3);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 6);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 4);

	brick_unlink(west_brick);
	refcount = brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 4);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 4);

	brick_unlink(east_brick);
	refcount = brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 1);

	/* destroy */
	brick_decref(west_brick);
	brick_decref(middle_brick);
	brick_decref(east_brick);
}

static void test_brick_core_verify_multiple_link(void)
{
	struct brick *west_brick, *middle_brick, *east_brick;
	uint32_t links_count;

	west_brick = brick_new("nop");
	middle_brick = brick_new("nop");
	east_brick = brick_new("nop");

	/* create a few links */
	brick_west_link(middle_brick, west_brick);
	brick_west_link(middle_brick, west_brick);
	brick_east_link(middle_brick, east_brick);
	brick_east_link(middle_brick, east_brick);
	brick_east_link(middle_brick, east_brick);

	/* check the link count */
	links_count = brick_links_count_get(west_brick, west_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, middle_brick);
	g_assert(links_count == 2);
	links_count = brick_links_count_get(west_brick, east_brick);
	g_assert(links_count == 0);

	links_count = brick_links_count_get(middle_brick, middle_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, west_brick);
	g_assert(links_count == 2);
	links_count = brick_links_count_get(middle_brick, east_brick);
	g_assert(links_count == 3);

	links_count = brick_links_count_get(east_brick, east_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, west_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, middle_brick);
	g_assert(links_count == 3);

	/* unlink the west brick */
	brick_unlink(west_brick);

	/* check again */
	links_count = brick_links_count_get(west_brick, west_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, middle_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, east_brick);
	g_assert(links_count == 0);

	links_count = brick_links_count_get(middle_brick, middle_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, west_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, east_brick);
	g_assert(links_count == 3);

	links_count = brick_links_count_get(east_brick, east_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, west_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, middle_brick);
	g_assert(links_count == 3);

	/* unlink the east brick */
	brick_unlink(east_brick);

	/* check again */
	links_count = brick_links_count_get(west_brick, west_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, middle_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, east_brick);
	g_assert(links_count == 0);

	links_count = brick_links_count_get(middle_brick, middle_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, west_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, east_brick);
	g_assert(links_count == 0);

	links_count = brick_links_count_get(east_brick, east_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, west_brick);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, middle_brick);
	g_assert(links_count == 0);

	/* destroy */
	brick_decref(west_brick);
	brick_decref(middle_brick);
	brick_decref(east_brick);
}

int main(int argc, char **argv)
{
	/* tests in the same order as the header function declarations */
	g_test_init(&argc, &argv, NULL);
	g_test_add_func("/brick/core/simple-lifecycle",
			test_brick_core_simple_lifecycle);
	g_test_add_func("/brick/core/refcount", test_brick_core_refcount);
	g_test_add_func("/brick/core/link",	test_brick_core_link);
	g_test_add_func("/brick/core/multiple-link",
			test_brick_core_multiple_link);
	g_test_add_func("/brick/core/verify/multiple-link",
			test_brick_core_verify_multiple_link);

	return g_test_run();
}
