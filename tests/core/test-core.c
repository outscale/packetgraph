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

#include <packetgraph/nop.h>
#include "utils/config.h"
#include "brick.h"
#include "tests.h"

static void test_brick_core_simple_lifecycle(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick;
	struct pg_brick_config *config = pg_brick_config_new("mybrick", 2, 2,
							     PG_MULTIPOLE);

	brick = pg_brick_new("foo", config, &error);
	g_assert(!brick);
	g_assert(error);
	g_assert(error->message);
	g_assert_cmpstr(error->message, ==, "Brick 'foo' not found");
	pg_error_free(error);
	error = NULL;

	brick = pg_brick_new("nop", config, &error);
	g_assert(brick);
	g_assert(!error);

	pg_brick_decref(brick, &error);
	g_assert(!error);

	brick = pg_brick_decref(NULL, &error);
	g_assert(!brick);
	g_assert(error);
	g_assert(error->message);
	g_assert_cmpstr(error->message, ==, "NULL brick");
	pg_error_free(error);
	error = NULL;

	pg_brick_config_free(config);
}

static void test_brick_core_refcount(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *brick;
	struct pg_brick_config *config = pg_brick_config_new("mybrick", 2, 2,
							     PG_MULTIPOLE);

	brick = pg_brick_new("nop", config, &error);
	g_assert(brick);
	g_assert(!error);

	/* Use the brick twice */
	pg_brick_incref(brick);
	pg_brick_incref(brick);

	/* Release it twice */
	brick = pg_brick_decref(brick, &error);
	g_assert(brick);
	g_assert(!error);
	brick = pg_brick_decref(brick, &error);
	g_assert(brick);
	g_assert(!error);

	/* finally destroy the brick */
	brick = pg_brick_decref(brick, &error);
	g_assert(!brick);
	g_assert(!error);

	pg_brick_config_free(config);
}

static void test_brick_core_link(void)
{
	struct pg_error *error = NULL;
	struct pg_brick *west_brick, *middle_brick, *east_brick;
	struct pg_brick_config *config = pg_brick_config_new("mybrick", 4, 4,
							     PG_MULTIPOLE);
	int64_t refcount;
	int ret;

	west_brick = pg_brick_new("nop", config, &error);
	g_assert(west_brick);
	g_assert(!error);

	middle_brick = pg_brick_new("nop", config, &error);
	g_assert(middle_brick);
	g_assert(!error);

	east_brick = pg_brick_new("nop", config, &error);
	g_assert(east_brick);
	g_assert(!error);

	ret = pg_brick_link(west_brick, middle_brick,  &error);
	g_assert(ret == 0);
	g_assert(!error);

	ret = pg_brick_link(middle_brick, east_brick, &error);
	g_assert(ret == 0);
	g_assert(!error);

	refcount = pg_brick_refcount(west_brick);
	g_assert(refcount == 2);
	refcount = pg_brick_refcount(middle_brick);
	g_assert(refcount == 3);
	refcount = pg_brick_refcount(east_brick);
	g_assert(refcount == 2);

	pg_brick_unlink(west_brick, &error);
	g_assert(!error);
	refcount = pg_brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = pg_brick_refcount(middle_brick);
	g_assert(refcount == 2);
	refcount = pg_brick_refcount(east_brick);
	g_assert(refcount == 2);

	pg_brick_unlink(east_brick, &error);
	g_assert(!error);
	refcount = pg_brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = pg_brick_refcount(middle_brick);
	g_assert(refcount == 1);
	refcount = pg_brick_refcount(east_brick);
	g_assert(refcount == 1);

	/* destroy */
	pg_brick_decref(west_brick, &error);
	g_assert(!error);
	pg_brick_decref(middle_brick, &error);
	g_assert(!error);
	pg_brick_decref(east_brick, &error);
	g_assert(!error);

	pg_brick_config_free(config);
}

static void test_brick_core_multiple_link(void)
{
	struct pg_brick *west_brick, *middle_brick, *east_brick;
	struct pg_brick_config *config = pg_brick_config_new("mybrick", 4, 4,
							     PG_MULTIPOLE);
	struct pg_error *error = NULL;
	int64_t refcount;
	int ret;

	west_brick = pg_brick_new("nop", config, &error);
	g_assert(west_brick);
	g_assert(!error);

	middle_brick = pg_brick_new("nop", config, &error);
	g_assert(middle_brick);
	g_assert(!error);

	east_brick = pg_brick_new("nop", config, &error);
	g_assert(east_brick);
	g_assert(!error);

	ret = pg_brick_link(west_brick, middle_brick, &error);
	g_assert(ret == 0);
	g_assert(!error);
	ret = pg_brick_link(west_brick, middle_brick, &error);
	g_assert(ret == 0);
	g_assert(!error);

	ret = pg_brick_link(middle_brick, east_brick, &error);
	g_assert(ret == 0);
	g_assert(!error);
	ret = pg_brick_link(middle_brick, east_brick, &error);
	g_assert(ret == 0);
	g_assert(!error);
	ret = pg_brick_link(middle_brick, east_brick, &error);
	g_assert(ret == 0);
	g_assert(!error);

	refcount = pg_brick_refcount(west_brick);
	g_assert(refcount == 3);
	refcount = pg_brick_refcount(middle_brick);
	g_assert(refcount == 6);
	refcount = pg_brick_refcount(east_brick);
	g_assert(refcount == 4);

	pg_brick_unlink(west_brick, &error);
	g_assert(!error);
	refcount = pg_brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = pg_brick_refcount(middle_brick);
	g_assert(refcount == 4);
	refcount = pg_brick_refcount(east_brick);
	g_assert(refcount == 4);

	pg_brick_unlink(east_brick, &error);
	g_assert(!error);
	refcount = pg_brick_refcount(west_brick);
	g_assert(refcount == 1);
	refcount = pg_brick_refcount(middle_brick);
	g_assert(refcount == 1);
	refcount = pg_brick_refcount(east_brick);
	g_assert(refcount == 1);

	/* destroy */
	pg_brick_decref(west_brick, &error);
	g_assert(!error);
	pg_brick_decref(middle_brick, &error);
	g_assert(!error);
	pg_brick_decref(east_brick, &error);
	g_assert(!error);

	pg_brick_config_free(config);
}

static int test_side_sanity_check(struct pg_brick_side *side,
				  enum pg_brick_type type)
{
	uint16_t j, count = 0;

	if (type == PG_MULTIPOLE) {
		for (j = 0; j < side->max; j++) {
			if (side->edges[j].link)
				count++;
		}
	} else if (side->edge.link != NULL) {
		count++;
	}
	g_assert(count == side->nb);
	return count;
}


static void test_brick_sanity_check(struct pg_brick *brick)
{
	test_side_sanity_check(&brick->sides[WEST_SIDE], brick->type);
	test_side_sanity_check(&brick->sides[EAST_SIDE], brick->type);
}

static void test_brick_sanity_check_expected(struct pg_brick *brick,
					     int west, int east)
{
	g_assert(test_side_sanity_check(&brick->sides[WEST_SIDE], brick->type)
		 == west);
	g_assert(test_side_sanity_check(&brick->sides[EAST_SIDE], brick->type)
		 == east);
}


static void test_brick_core_verify_multiple_link(void)
{
	struct pg_brick *west_brick, *middle_brick, *east_brick;
	struct pg_brick_config *config = pg_brick_config_new("mybrick", 4, 4,
							     PG_MULTIPOLE);
	uint32_t links_count;
	struct pg_error *error = NULL;

	west_brick = pg_brick_new("nop", config, &error);
	g_assert(!error);
	middle_brick = pg_brick_new("nop", config, &error);
	g_assert(!error);
	east_brick = pg_brick_new("nop", config, &error);
	g_assert(!error);

	/* create a few links */
	pg_brick_link(west_brick, middle_brick, &error);
	g_assert(!error);
	pg_brick_link(west_brick, middle_brick, &error);
	g_assert(!error);
	pg_brick_link(middle_brick, east_brick, &error);
	g_assert(!error);
	pg_brick_link(middle_brick, east_brick, &error);
	g_assert(!error);
	pg_brick_link(middle_brick, east_brick, &error);
	g_assert(!error);

	/* sanity checks */
	test_brick_sanity_check(west_brick);
	test_brick_sanity_check(middle_brick);
	test_brick_sanity_check(east_brick);

	/* check the link count */
	links_count = pg_brick_links_count_get(west_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(west_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 2);
	links_count = pg_brick_links_count_get(west_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	links_count = pg_brick_links_count_get(middle_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(middle_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 2);
	links_count = pg_brick_links_count_get(middle_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 3);

	links_count = pg_brick_links_count_get(east_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(east_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(east_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 3);

	/* unlink the west brick */
	pg_brick_unlink(west_brick, &error);

	/* sanity checks */
	test_brick_sanity_check(west_brick);
	test_brick_sanity_check(middle_brick);
	test_brick_sanity_check(east_brick);

	/* check again */
	links_count = pg_brick_links_count_get(west_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(west_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(west_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	links_count = pg_brick_links_count_get(middle_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(middle_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(middle_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 3);

	links_count = pg_brick_links_count_get(east_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(east_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(east_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 3);

	/* unlink the east brick */
	pg_brick_unlink(east_brick, &error);
	g_assert(!error);

	/* sanity checks */
	test_brick_sanity_check(west_brick);
	test_brick_sanity_check(middle_brick);
	test_brick_sanity_check(east_brick);

	/* check again */
	links_count = pg_brick_links_count_get(west_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(west_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(west_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	links_count = pg_brick_links_count_get(middle_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(middle_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(middle_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	links_count = pg_brick_links_count_get(east_brick, east_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(east_brick, west_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);
	links_count = pg_brick_links_count_get(east_brick, middle_brick, &error);
	g_assert(!error);
	g_assert(links_count == 0);

	/* destroy */
	pg_brick_decref(west_brick, &error);
	g_assert(!error);
	pg_brick_decref(middle_brick, &error);
	g_assert(!error);
	pg_brick_decref(east_brick, &error);
	g_assert(!error);

	pg_brick_config_free(config);
}

static void test_brick_core_verify_re_link(void)
{
	struct pg_error *e = NULL;
	struct pg_brick *v = pg_nop_new("v", &e);
	g_assert(!e);
	struct pg_brick *f = pg_nop_new("f", &e);
	g_assert(!e);
	struct pg_brick *a = pg_nop_new("a", &e);
	g_assert(!e);
	struct pg_brick *s = pg_nop_new("s", &e);
	g_assert(!e);

	/* Initial state: v -- f -- a */
	pg_brick_chained_links(&e, v, f, a);
	g_assert(!e);
	test_brick_sanity_check_expected(v, 0, 1);
	test_brick_sanity_check_expected(f, 1, 1);
	test_brick_sanity_check_expected(a, 1, 0);

	/* Unlink f */
	pg_brick_unlink(f, &e);
	g_assert(!e);
	test_brick_sanity_check_expected(v, 0, 0);
	test_brick_sanity_check_expected(f, 0, 0);
	test_brick_sanity_check_expected(a, 0, 0);

	/* Link v and s */
	pg_brick_link(v, s, &e);
	g_assert(!e);
	test_brick_sanity_check_expected(v, 0, 1);
	test_brick_sanity_check_expected(s, 1, 0);
	test_brick_sanity_check_expected(f, 0, 0);
	test_brick_sanity_check_expected(a, 0, 0);

	/* link the rest to have v -- s -- f -- a */
	pg_brick_link(s, f, &e);
	g_assert(!e);
	test_brick_sanity_check_expected(v, 0, 1);
	test_brick_sanity_check_expected(s, 1, 1);
	test_brick_sanity_check_expected(f, 1, 0);
	test_brick_sanity_check_expected(a, 0, 0);

	pg_brick_link(f, a, &e);
	g_assert(!e);
	test_brick_sanity_check_expected(v, 0, 1);
	test_brick_sanity_check_expected(s, 1, 1);
	test_brick_sanity_check_expected(f, 1, 1);
	test_brick_sanity_check_expected(a, 1, 0);

	pg_brick_destroy(a);
	pg_brick_destroy(v);
	pg_brick_destroy(f);
	pg_brick_destroy(s);
}

void test_brick_core(void)
{
	/* tests in the same order as the header function declarations */
	g_test_add_func("/core/simple-lifecycle",
			test_brick_core_simple_lifecycle);
	g_test_add_func("/core/refcount", test_brick_core_refcount);
	g_test_add_func("/core/link",	test_brick_core_link);
	g_test_add_func("/core/multiple-link",
			test_brick_core_multiple_link);
	g_test_add_func("/core/verify/multiple-link",
			test_brick_core_verify_multiple_link);
	g_test_add_func("/core/verify/re-link",
			test_brick_core_verify_re_link);


}
