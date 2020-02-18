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

#define CATCAT(a, b, c) a##b##c
#define CAT(a, b) a##b
#define ST_(t) CAT(pg_mac_table_, t)
#define TARGET_(t) CAT(t, s)
#define ST ST_(MAC_TABLE_TYPE)
#define TARGET TARGET_(MAC_TABLE_TYPE)

#define pg_mac_table_iterator_init_(t) CATCAT(pg_mac_table_iterator_, t, _init)
#define pg_mac_table_iterator_init pg_mac_table_iterator_init_(MAC_TABLE_TYPE)

static inline void
pg_mac_table_iterator_init(struct pg_mac_table_iterator *it,
			   struct pg_mac_table *tbl,
			   void next(struct pg_mac_table_iterator *))
{
	it->i = 0;
	it->j = 0;
	it->mj = 0;
	it->mi = 0;
	it->m0 = tbl->mask[0];
	it->tbl = tbl;
	if (unlikely(it->m0 & 1)) {
		it->m1 = tbl->elems[0]->mask[0];
		if (unlikely(it->m1 & 1)) {
			MAC_TABLE_SETTER(tbl->TARGET[0], 0);
			it->m1 &= ~1;
			return;
		}
	} else {
		it->m1 = 0;
	}

	/* If 0 is set we don't need to increment the iterator */
	next(it);
}

#define pg_mac_table_iterator_create_(t)		\
	CATCAT(pg_mac_table_iterator_, t, _create)

#define pg_mac_table_iterator_create			\
	pg_mac_table_iterator_create_(MAC_TABLE_TYPE)

static inline struct pg_mac_table_iterator
pg_mac_table_iterator_create(struct pg_mac_table *tbl,
			     void next(struct pg_mac_table_iterator *))
{
	struct pg_mac_table_iterator ret;

	pg_mac_table_iterator_init(&ret, tbl, next);
	return ret;
}

static inline void
MAC_TABLE_IT_NEXT(struct pg_mac_table_iterator *it)
{
	struct pg_mac_table *ma = it->tbl;
	/* So it seems gcc does some kind of loop unrolling and
	 * go above PG_MAC_TABLE_MASK_SIZE, making 'i' volatile fix it...
	 * I've made j volatile too because if 'i' need it
	 * j' should might it too */
	volatile uint32_t i = it->i;
	volatile uint32_t j = it->j;
	uint64_t m0 = it->m0;
	uint64_t m1 = it->m1;
	int been_out = 0;
	int mi = it->mi;

	for (;i < PG_MAC_TABLE_MASK_SIZE ; m0 = ma->mask[i]) {
		/* mi is undefine if m0 is 0,
		 * but if it's 0 we should skip the block */
		for(;({mi = ctz64(m0); m0;});) {
			struct ST *imt = ma->TARGET[i * 64 + mi];

			for (;j < PG_MAC_TABLE_MASK_SIZE; ++j) {

				if (been_out)
					m1 = imt->mask[j];
				for (;m1;) {
					int mj;

					pg_low_bit_iterate(m1, mj);
					MAC_TABLE_SETTER(imt, j * 64 + mj);
					it->mi = mi;
					it->j = j;
					it->mj = mj;
					it->i = i;
					it->m0 = m0;
					it->m1 = m1;

					return;
				}
				been_out = 1;
			}
			j = 0;
			m0 &= ~ (1LLU << mi);
		}
		++i;
	}
	it->i = UINT32_MAX;
}

#undef CAT
#undef CATCAT
#undef ST
#undef TARGET
#undef ST_
#undef TARGET_

/* external args */
#undef MAC_TABLE_SETTER
#undef MAC_TABLE_IT_NEXT
#undef MAC_TABLE_TYPE
