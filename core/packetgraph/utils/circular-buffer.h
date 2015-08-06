/* Copyright 2015 Outscale SAS
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

#ifndef _CIRCULAR_BUFFER_H_
#define _CIRCULAR_BUFFER_H_

#define REGISTER_CB(name, type, size)					\
	struct name {							\
		type buff[size];					\
		unsigned int start;					\
		unsigned int buff_size;					\
	};								\
	static const unsigned int name##_size = size;			\
	static const unsigned int name##_type_size = sizeof(type);


#define cb_init(cb, name) do {			\
		(cb).start = 0;			\
		(cb).buff_size = name##_size ;	\
	} while (0)

#define cb_init0(cb, name) do {						\
		cb_init((cb), name);					\
		memset((cb).buff, 0, name##_type_size * name##_size);	\
	} while (0)


#define cb_get(cb, i) ((cb).buff[((cb).start + (i)) % (cb).buff_size])

#define cb_set_start(cb, i) ((cb).start = (i))

#define cb_start_incr(cb) ((cb).start += 1)

#define cb_set(cb, i, val) (cb_get((cb), (i)) = val)

#endif
