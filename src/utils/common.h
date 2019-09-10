/* Copyright 2019 Outscale SAS
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

#ifndef PG_UTILS_COMMON_H_
#define PG_UTILS_COMMON_H_

#include <packetgraph/packetgraph.h>
#include <stdlib.h>

#define pg_cleanup(func)			\
	__attribute__((__cleanup__(func)))

static inline void pg_autofree_(void *p)
{
	void **pp = p;

	free(*pp);
}

#define pg_autofree				\
	__attribute__((__cleanup__(pg_autofree_)))


static inline void pg_brick_ptrptr_destroy(struct pg_brick **brick)
{
	pg_brick_destroy(*brick);
}

#define PG_STRCAT2(a, b) (a b)
#define PG_STRCAT3(a, b, c) (a b c)
#define PG_STRCAT4(a, b, c, d) (a b c d)
#define PG_STRCAT5(a, b, c, d, e) (a b c d e)
#define PG_STRCAT6(a, b, c, d, e, f) (a b c d e f)
#define PG_STRCAT7(a, b, c, d, e, f, g) (a b c d e f g)
#define PG_STRCAT8(a, b, c, d, e, f, g, h) (a b c d e f g h)
#define PG_STRCAT9(a, b, c, d, e, f, g, h, i) (a b c d e f g h i)
#define PG_STRCAT10(a, b, c, d, e, f, g, h, i, j) (a b c d e f g h i j)

#define PG_CAT(a, b) a ## b

#define PG_STRCAT_(nb, ...) (PG_CAT(PG_STRCAT, nb) (__VA_ARGS__))	\

#define PG_STRCAT(...)						\
	PG_STRCAT_(PG_GET_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

#define PG_GET_ARG_COUNT(...) PG__GET_ARG_COUNT(			\
		0, ## __VA_ARGS__, 70, 69, 68,				\
		67, 66, 65, 64, 63, 62, 61, 60,				\
		59, 58, 57, 56, 55, 54, 53, 52,				\
		51, 50, 49, 48, 47, 46, 45, 44,				\
		43, 42, 41, 40, 39, 38, 37, 36,				\
		35, 34, 33, 32, 31, 30, 29, 28,				\
		27, 26, 25, 24, 23, 22, 21, 20,				\
		19, 18, 17, 16, 15, 14, 13, 12,				\
		11, 10, 9, 8, 7, 6, 5, 4, 3, 2,				\
		1, 0)

#define PG__GET_ARG_COUNT(						\
	_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_,		\
	_11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_,		\
	_20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_,		\
	_29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37,		\
	_38, _39, _40, _41, _42, _43, _44, _45, _46, _47,		\
	_48, _49, _50, _51, _52, _53, _54, _55, _56, _57,		\
	_58, _59, _60, _61, _62, _63, _64, _65, _66, _67,		\
	_68, _69, _70, count, ...) count

#endif /* PG_UTILS_COMMON_H_ */
