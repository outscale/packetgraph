/*-
 * Copyright (c) 2015 Mindaugas Rasiukevicius <rmind at netbsd org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PG_FIREWALL_NPF_DPDK_H
#define _PG_FIREWALL_NPF_DPDK_H

#include <net/npf.h>
#include <net/npfkern.h>
#include <net/if.h>
#include <npf.h>

#define CONCAT(X, Y) X##Y
#define TYPEDEF CONCAT(type, def)

TYPEDEF struct ifnet {
	struct if_nameindex ini;
	void *arg;

	LIST_ENTRY(ifnet) entry;
} ifnet_t;

#undef CONCAT
#undef TYPEDEF

struct rte_mempool;

void npf_dpdk_init(struct rte_mempool *);
npf_t *npf_dpdk_create(int);
struct ifnet *npf_dpdk_ifattach(npf_t *, const char *, unsigned);
void npf_dpdk_ifdetach(npf_t *, struct ifnet *);

#endif /* _PG_FIREWALL_NPF_DPDK_H */

