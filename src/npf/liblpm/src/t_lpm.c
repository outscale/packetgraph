/*
 * This file is in the Public Domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#include "lpm.h"

static void
ipv4_basic_test(void)
{
	lpm_t *lpm;
	uint32_t addr;
	size_t len;
	unsigned pref;
	void *val;
	int ret;

	lpm = lpm_create();
	assert(lpm != NULL);

	/*
	 * Simple /32 cases.
	 */

	lpm_strtobin("10.0.0.1", &addr, &len, &pref);
	ret = lpm_insert(lpm, &addr, len, pref, (void *)0xa);
	assert(ret == 0);

	lpm_strtobin("10.0.0.1", &addr, &len, &pref);
	ret = lpm_insert(lpm, &addr, len, pref, (void *)0xb);
	assert(ret == 0);

	lpm_strtobin("10.0.0.1", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == (void *)0xb);

	lpm_strtobin("10.0.0.2", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == NULL);

	/*
	 * Single /24 network.
	 */

	lpm_strtobin("10.1.1.0/24", &addr, &len, &pref);
	ret = lpm_insert(lpm, &addr, len, pref, (void *)0x100);
	assert(ret == 0);

	lpm_strtobin("10.1.1.64", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == (void *)0x100);

	/*
	 * Two adjacent /25 networks.
	 */

	lpm_strtobin("10.2.2.0/25", &addr, &len, &pref);
	ret = lpm_insert(lpm, &addr, len, pref, (void *)0x101);
	assert(ret == 0);

	lpm_strtobin("10.2.2.128/25", &addr, &len, &pref);
	ret = lpm_insert(lpm, &addr, len, pref, (void *)0x102);
	assert(ret == 0);

	lpm_strtobin("10.2.2.127", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == (void *)0x101);

	lpm_strtobin("10.2.2.255", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == (void *)0x102);

	/*
	 * Longer /31 prefix in /29.
	 */

	lpm_strtobin("10.3.3.60/31", &addr, &len, &pref);
	ret = lpm_insert(lpm, &addr, len, pref, (void *)0x103);
	assert(ret == 0);

	lpm_strtobin("10.3.3.56/29", &addr, &len, &pref);
	ret = lpm_insert(lpm, &addr, len, pref, (void *)0x104);
	assert(ret == 0);

	lpm_strtobin("10.3.3.61", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == (void *)0x103);

	lpm_strtobin("10.3.3.62", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == (void *)0x104);

	/*
	 * No match (out of range by one).
	 */

	lpm_strtobin("10.3.3.55", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == NULL);

	lpm_strtobin("10.3.3.64", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == NULL);

	/*
	 * Default.
	 */
	lpm_strtobin("0.0.0.0/0", &addr, &len, &pref);
	ret = lpm_insert(lpm, &addr, len, pref, (void *)0x105);
	assert(ret == 0);

	lpm_strtobin("10.3.3.64", &addr, &len, &pref);
	val = lpm_lookup(lpm, &addr, len);
	assert(val == (void *)0x105);

	lpm_destroy(lpm);
}

static void
ipv4_basic_random(void)
{
	const unsigned nitems = 1024;
	lpm_t *lpm;
	uint32_t addr[nitems];

	lpm = lpm_create();
	assert(lpm != NULL);

	for (unsigned i = 0; i < nitems; i++) {
		uint32_t x;
		int ret;

		x = addr[i] = random();
		ret = lpm_insert(lpm, &addr[i], 4, 32, (void *)(uintptr_t)x);
		assert(ret == 0);
	}
	for (unsigned i = 0; i < nitems; i++) {
		const uint32_t x = addr[i];
		void *val = lpm_lookup(lpm, &x, 4);
		assert((uintptr_t)val == x);
	}

	lpm_destroy(lpm);
}

static void
ipv6_basic_test(void)
{
	lpm_t *lpm;
	uint32_t addr[4];
	size_t len;
	unsigned pref;
	void *val;
	int ret;

	lpm = lpm_create();
	assert(lpm != NULL);

	lpm_strtobin("abcd:8000::1", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == NULL);

	lpm_strtobin("::/0", addr, &len, &pref);
	ret = lpm_insert(lpm, addr, len, pref, (void *)0x100);
	assert(ret == 0);

	lpm_strtobin("abcd:8000::1", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x100);

	lpm_strtobin("abcd:8000::/17", addr, &len, &pref);
	ret = lpm_insert(lpm, addr, len, pref, (void *)0x101);
	assert(ret == 0);

	lpm_strtobin("abcd:8000::1", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x101);

	lpm_strtobin("abcd::/16", addr, &len, &pref);
	ret = lpm_insert(lpm, addr, len, pref, (void *)0x102);
	assert(ret == 0);

	lpm_strtobin("abcd:abcd::/32", addr, &len, &pref);
	ret = lpm_insert(lpm, addr, len, pref, (void *)0x103);
	assert(ret == 0);

	lpm_strtobin("dddd:abcd::abcd/128", addr, &len, &pref);
	ret = lpm_insert(lpm, addr, len, pref, (void *)0x104);
	assert(ret == 0);

	lpm_strtobin("aaaa:bbbb:cccc:dddd:abcd:8000::/81", addr, &len, &pref);
	ret = lpm_insert(lpm, addr, len, pref, (void *)0x105);
	assert(ret == 0);

	lpm_strtobin("aaaa:bbbb:cccc:dddd:abcd::/80", addr, &len, &pref);
	ret = lpm_insert(lpm, addr, len, pref, (void *)0x106);
	assert(ret == 0);

	lpm_strtobin("ffff:ffff:ffff:ffff::/64", addr, &len, &pref);
	ret = lpm_insert(lpm, addr, len, pref, (void *)0x107);
	assert(ret == 0);

	lpm_strtobin("abcd:ffff::abcd", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x101);

	lpm_strtobin("abcd:0000::abcd", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x102);

	lpm_strtobin("abcd:abcd::abc0", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x103);

	lpm_strtobin("dddd:abcd::abcd", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x104);

	lpm_strtobin("aaaa:bbbb:cccc:dddd:abcd:ffff::abcd", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x105);

	lpm_strtobin("aaaa:bbbb:cccc:dddd:abcd::abcd", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x106);

	lpm_strtobin("ffff:ffff:ffff:ffff::abcd", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x107);

	lpm_strtobin("ffff:ffff:ffff:ffff::/64", addr, &len, &pref);
	ret = lpm_remove(lpm, addr, len, pref);
	assert(ret == 0);

	lpm_strtobin("ffff:ffff:ffff:fffe::abcd", addr, &len, &pref);
	val = lpm_lookup(lpm, addr, len);
	assert(val == (void *)0x100);

	lpm_destroy(lpm);
}

int
main(void)
{
	ipv4_basic_test();
	ipv4_basic_random();
	ipv6_basic_test();
	puts("ok");
	return 0;
}
