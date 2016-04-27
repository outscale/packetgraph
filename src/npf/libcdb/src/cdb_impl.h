/*
 * This file is in the Public Domain.
 */

#ifndef	_CDB_IMPL_H
#define	_CDB_IMPL_H

#ifndef __NetBSD__

#define	__predict_false(exp)	__builtin_expect((exp) != 0, 0)

static inline uint32_t
le32dec(const void *buf)
{
	const uint8_t *p = (const uint8_t *)buf;

	return ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

static inline void
le32enc(void *buf, uint32_t u)
{
	uint8_t *p = (uint8_t *)buf;

	p[0] = u & 0xff;
	p[1] = (u >> 8) & 0xff;
	p[2] = (u >> 16) & 0xff;
	p[3] = (u >> 24) & 0xff;
}

void	 mi_vector_hash(const void * __restrict, size_t, uint32_t, uint32_t[3]);

#endif

#endif
