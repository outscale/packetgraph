/*
 *Copyright (C) 2016 Matthias Gatto
 *
 *This program is free software: you can redistribute it and/or modify
 *it under the terms of the GNU Lesser General Public License as published by
 *the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU Lesser General Public License
 *along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PG_UTILS_STACK_H
#define _PG_UTILS_STACK_H


#define PG_STACK_BLOCK_SIZE 126

struct int8_stack {
	int8_t *values;
	size_t len;
	size_t total_size;
	size_t limit;
} int8_stack;

struct int16_stack {
	int16_t *values;
	size_t len;
	size_t total_size;
	size_t limit;
} int16_stack;

struct int32_stack {
	int32_t *values;
	size_t len;
	size_t total_size;
	size_t limit;
} int32_stack;

struct int64_stack {
	int64_t *values;
	size_t len;
	size_t total_size;
	size_t limit;
} int64_stack;

struct ptr_stack {
	void **values;
	size_t len;
	size_t total_size;
	size_t limit;
} ptr_stack;

#define STACK_CREATE(name, type)			\
	struct type##_stack name = {NULL, 0, 0, 0}	\

#define stack_push(stack, val)	do {					\
		if (stack.limit && (stack.len + 1) >= stack.limit)	\
			break;						\
		if (unlikely(stack.len == stack.total_size)) {		\
			stack.total_size += PG_STACK_BLOCK_SIZE;	\
			stack.values = realloc(stack.values,		\
					       stack.total_size *	\
					       sizeof(val));		\
		}							\
		stack.values[stack.len] = val;				\
		stack.len += 1;						\
	} while (0)

#define stack_len(stack) ((stack).len)

#define stack_set_limit(stack, l) (stack.limit = l)

static inline int8_t int8_stack_pop(struct int8_stack *stack,
				    int8_t fail_ret)
{
	if (!stack->len)
		return fail_ret;
	stack->len -= 1;
	return stack->values[stack->len];
}

static inline int16_t int16_stack_pop(struct int16_stack *stack,
				      int16_t fail_ret)
{
	if (!stack->len)
		return fail_ret;
	stack->len -= 1;
	return stack->values[stack->len];
}

static inline int32_t int32_stack_pop(struct int32_stack *stack,
				      int32_t fail_ret)
{
	if (!stack->len)
		return fail_ret;
	stack->len -= 1;
	return stack->values[stack->len];
}

static inline int64_t int64_stack_pop(struct int64_stack *stack,
				      int64_t fail_ret)
{
	if (!stack->len)
		return fail_ret;
	stack->len -= 1;
	return stack->values[stack->len];
}

static inline void *ptr_stack_pop(struct ptr_stack *stack, void *fail_ret)
{
	if (!stack->len)
		return fail_ret;
	stack->len -= 1;
	return stack->values[stack->len];
}

/**
 * @brief pop a value from @stack and return it
 * @stack the stack
 * @fail_val the value assigne to ret if stack is empty
 */
#define stack_pop(stack, fail_val)			\
	(_Generic((stack),				\
		  struct int8_stack : int8_stack_pop,	\
		  struct int16_stack : int16_stack_pop,	\
		  struct int32_stack : int32_stack_pop,	\
		  struct int64_stack : int64_stack_pop,	\
		  struct ptr_stack : ptr_stack_pop	\
		)(&(stack), fail_val))

#define stack_destroy(stack) do {		\
		free(stack.values);		\
		stack.values = NULL;		\
		stack.len = 0;			\
		stack.total_size = 0;		\
} while (0)

#endif
