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

#include <packetgraph/brick.h>
#include <packetgraph/utils/config.h>
#include "tests.h"

static void test_brick_core_simple_lifecycle(void)
{
	struct switch_error *error = NULL;
	struct brick *brick;
	struct brick_config *config = brick_config_new("mybrick", 2, 2);

	brick = brick_new("foo", config, &error);
	g_assert(!brick);
	g_assert(error);
	g_assert(error->message);
	g_assert_cmpstr(error->message, ==, "Brick 'foo' not found");
	error_free(error);
	error = NULL;

	brick = brick_new("nop", config, &error);
	g_assert(brick);
	g_assert(!error);

	brick_decref(brick, &error);
	g_assert(!error);

	brick = brick_decref(NULL, &error);
	g_assert(!brick);
	g_assert(error);
	g_assert(error->message);
	g_assert_cmpstr(error->message, ==, "NULL brick");
	error_free(error);
	error = NULL;

	brick_config_free(config);
}

static void test_brick_core_refcount(void)
{
	struct switch_error *error = NULL;
	struct brick *brick;
	struct brick_config *config = brick_config_new("mybrick", 2, 2);

	brick = brick_new("nop", config, &error);
	g_assert(brick);
	g_assert(!error);

	/* Use the brick twice */
	brick_incref(brick);
	brick_incref(brick);

	/* Release it twice */
	brick = brick_decref(brick, &error);
	g_assert(brick);
	g_assert(!error);
	brick = brick_decref(brick, &error);
	g_assert(brick);
	g_assert(!error);

	/* finally destroy the brick */
	brick = brick_decref(brick, &error);
	g_assert(!brick);
	g_assert(!error);

	brick_config_free(config);
}

static void test_brick_core_link(void)
{
	struct switch_error *error = NULL;
	struct brick *west_brick, *middle_brick, *east_brick;
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	int64_t refcount;
	int ret;

	west_brick = brick_new("nop", config, &error);
	g_assert(west_brick);
	g_assert(!error);

	middle_brick = brick_new("nop", config, &error);
	g_assert(middle_brick);
	g_assert(!error);

	east_brick = brick_new("nop", config, &error);
	g_assert(east_brick);
	g_assert(!error);

	ret = brick_link(west_brick, middle_brick,  &error);
	g_assert(ret);
	g_assert(!error);

	ret = brick_link(middle_brick, east_brick, &error);
	g_assert(ret);
	g_assert(!error);

	refcount = brick_refcount(west_brick);
	g_assert(refcount == 2);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 3);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 2);

	brick_unlink(west_brick, &error);
	g_assert(!error);
	refcount = brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 2);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 2);

	brick_unlink(east_brick, &error);
	g_assert(!error);
	refcount = brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 1);

	/* destroy */
	brick_decref(west_brick, &error);
	g_assert(!error);
	brick_decref(middle_brick, &error);
	g_assert(!error);
	brick_decref(east_brick, &error);
	g_assert(!error);

	brick_config_free(config);
}

static void test_brick_core_multiple_link(void)
{
	struct brick *west_brick, *middle_brick, *east_brick;
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	struct switch_error *error = NULL;
	int64_t refcount;
	int ret;

	west_brick = brick_new("nop", config, &error);
	g_assert(west_brick);
	g_assert(!error);

	middle_brick = brick_new("nop", config, &error);
	g_assert(middle_brick);
	g_assert(!error);

	east_brick = brick_new("nop", config, &error);
	g_assert(east_brick);
	g_assert(!error);

	ret = brick_link(west_brick, middle_brick, &error);
	g_assert(ret);
	g_assert(!error);
	ret = brick_link(west_brick, middle_brick, &error);
	g_assert(ret);
	g_assert(!error);

	ret = brick_link(middle_brick, east_brick, &error);
	g_assert(ret);
	g_assert(!error);
	ret = brick_link(middle_brick, east_brick, &error);
	g_assert(ret);
	g_assert(!error);
	ret = brick_link(middle_brick, east_brick, &error);
	g_assert(ret);
	g_assert(!error);

	refcount = brick_refcount(west_brick);
	g_assert(refcount == 3);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 6);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 4);

	brick_unlink(west_brick, &error);
	g_assert(!error);
	refcount = brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 4);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 4);

	brick_unlink(east_brick, &error);
	g_assert(!error);
	refcount = brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(middle_brick);
	g_assert(refcount == 1);
	refcount = brick_refcount(east_brick);
	g_assert(refcount == 1);

	/* destroy */
	brick_decref(west_brick, &error);
	g_assert(!error);
	brick_decref(middle_brick, &error);
	g_assert(!error);
	brick_decref(east_brick, &error);
	g_assert(!error);

	brick_config_free(config);
}

static void test_brick_core_verify_multiple_link(void)
{
	struct brick *west_brick, *middle_brick, *east_brick;
	struct brick_config *config = brick_config_new("mybrick", 4, 4);
	uint32_t links_count;
	struct switch_error *error = NULL;

	west_brick = brick_new("nop", config, &error);
	g_assert(!error);
	middle_brick = brick_new("nop", config, &error);
	g_assert(!error);
	east_brick = brick_new("nop", config, &error);
	g_assert(!error);

	/* create a few links */
	brick_link(west_brick, middle_brick, &error);
	g_assert(!error);
	brick_link(west_brick, middle_brick, &error);
	g_assert(!error);
	brick_link(middle_brick, east_brick, &error);
	g_assert(!error);
	brick_link(middle_brick, east_brick, &error);
	g_assert(!error);
	brick_link(middle_brick, east_brick, &error);
	g_assert(!error);

	/* check the link count */
	links_count = brick_links_count_get(west_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 2);
	links_count = brick_links_count_get(west_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	links_count = brick_links_count_get(middle_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 2);
	links_count = brick_links_count_get(middle_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 3);

	links_count = brick_links_count_get(east_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 3);

	/* unlink the west brick */
	brick_unlink(west_brick, &error);

	/* check again */
	links_count = brick_links_count_get(west_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	links_count = brick_links_count_get(middle_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 3);

	links_count = brick_links_count_get(east_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 3);

	/* unlink the east brick */
	brick_unlink(east_brick, &error);
	g_assert(!error);

	/* check again */
	links_count = brick_links_count_get(west_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(west_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	links_count = brick_links_count_get(middle_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(middle_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	links_count = brick_links_count_get(east_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = brick_links_count_get(east_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	/* destroy */
	brick_decref(west_brick, &error);
	g_assert(!error);
	brick_decref(middle_brick, &error);
	g_assert(!error);
	brick_decref(east_brick, &error);
	g_assert(!error);

	brick_config_free(config);
}

void test_brick_core(void)
{
	/* tests in the same order as the header function declarations */
	g_test_add_func("/brick/core/simple-lifecycle",
			test_brick_core_simple_lifecycle);
	g_test_add_func("/brick/core/refcount", test_brick_core_refcount);
	g_test_add_func("/brick/core/link",	test_brick_core_link);
	g_test_add_func("/brick/core/multiple-link",
			test_brick_core_multiple_link);
	g_test_add_func("/brick/core/verify/multiple-link",
			test_brick_core_verify_multiple_link);

}
