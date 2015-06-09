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
#include <net/if.h>
#include <net/ethernet.h>
#include <glib.h>
#include <rte_config.h>
#include "bricks/brick.h"
#include "utils/bitmask.h"
#include "npf/dpdk/npf_dpdk.h"
#include <pcap/pcap.h>
#include <endian.h>

#define FIREWALL_SIDE_TO_NPF(side) \
	((side) == WEST_SIDE ? PFIL_OUT : PFIL_IN)

struct ifnet;

struct firewall_state {
	struct brick brick;
	npf_t *npf;
	struct ifnet *ifp;
	GList *rules;
};

static int firewall_build_pcap_filter(nl_rule_t *rl, const char *filter)
{
	const size_t maxsnaplen = 64 * 1024;
	struct bpf_program bf;
	size_t len;
	int ret;

	/* compile the expression (use DLT_RAW for NPF rules). */
	ret = pcap_compile_nopcap(maxsnaplen, DLT_RAW, &bf,
				  filter, 1, PCAP_NETMASK_UNKNOWN);
	if (ret)
		return ret;

	/* assign the byte-code to this rule. */
	len = bf.bf_len * sizeof(struct bpf_insn);
	ret = npf_rule_setcode(rl, NPF_CODE_BPF, bf.bf_insns, len);
	g_assert(ret == 0);
	pcap_freecode(&bf);
	return 0;
}

static inline int firewall_side_to_npf_rule(enum side side)
{
	switch (side) {
	case WEST_SIDE: return NPF_RULE_OUT;
	case EAST_SIDE: return NPF_RULE_IN;
	default: return NPF_RULE_OUT | NPF_RULE_IN;
	}
}

int firewall_rule_add(struct brick *brick, const char *filter, enum side dir,
		      int stateful, struct switch_error **errp)
{
	struct firewall_state *state;
	struct nl_rule *rule;
	int options = 0;

	state = brick_get_state(brick, struct firewall_state);
	options |= firewall_side_to_npf_rule(dir);
	if (stateful)
		options |= NPF_RULE_STATEFUL;
	rule = npf_rule_create(NULL, NPF_RULE_PASS | options, NULL);
	g_assert(rule);
	npf_rule_setprio(rule, NPF_PRI_LAST);
	if (filter && firewall_build_pcap_filter(rule, filter)) {
		*errp = error_new("pcap filter build failed");
		return 1;
	}
	state->rules = g_list_append(state->rules, rule);
	return 0;
}

int firewall_rule_flush(struct brick *brick)
{
	struct firewall_state *state;
	GList *it;

	state = brick_get_state(brick, struct firewall_state);
	/* clean all rules */
	it = state->rules;
	while (it) {
		npf_rule_destroy(it->data);
		it = g_list_next(it);
	}
	/* flush list */
	g_list_free(state->rules);
	state->rules = NULL;
	return 0;
}

int firewall_reload(struct brick *brick)
{
	npf_error_t errinfo;
	struct firewall_state *state;
	struct nl_config *config;
	void *config_build;
	int ret;
	GList *it;

	state = brick_get_state(brick, struct firewall_state);
	config = npf_config_create();

	it = state->rules;
	while (it != NULL) {
		npf_rule_insert(config, NULL, it->data);
		it = g_list_next(it);
	}

	config_build = npf_config_build(config);
	ret = npf_load(state->npf, config_build, &errinfo);
	npf_config_destroy(config);
	return ret;
}

struct brick *firewall_new(const char *name, uint32_t west_max,
			   uint32_t east_max, struct switch_error **errp)
{
	struct brick_config *config;
	struct brick *ret;

	config = brick_config_new(name, west_max, east_max);
	ret = brick_new("firewall", config, errp);
	brick_config_free(config);
	return ret;
}

static int firewall_burst(struct brick *brick, enum side side,
			  uint16_t edge_index, struct rte_mbuf **pkts,
			  uint16_t nb, uint64_t pkts_mask,
			  struct switch_error **errp)
{
	struct brick_side *s;
	struct firewall_state *state;
	int pf_side;
	uint64_t it_mask;
	uint64_t bit;
	uint16_t i;
	int ret;
	struct rte_mbuf *tmp;
	struct ether_header *eth;

	s = &brick->sides[flip_side(side)];
	state = brick_get_state(brick, struct firewall_state);
	pf_side = FIREWALL_SIDE_TO_NPF(side);

	/* npf-dpdk free filtered packets, we want to keep them */
	packets_incref(pkts, pkts_mask);

	it_mask = pkts_mask;
	for (; it_mask;) {
		low_bit_iterate_full(it_mask, bit, i);

		/* Firewall only manage IPv4 or IPv6 filtering.
		 * Let non-ip packets (like ARP) pass.
		 */
		eth = rte_pktmbuf_mtod(pkts[i], struct ether_header*);
		if (eth->ether_type != htobe16(ETHERTYPE_IP) &&
		    eth->ether_type != htobe16(ETHERTYPE_IPV6)) {
			continue;
		}

		/* NPF only manage layer 3 so we temporaly cut off layer 2.
		 * Note that this trick is not thread safe. To do so, we will
		 * have to clone packets just for filtering and will have to
		 * restroy cloned packets after handling them in NPF.
		 */
		rte_pktmbuf_adj(pkts[i], sizeof(struct ether_header));

		/* filter packet */
		tmp = pkts[i];
		ret = npf_packet_handler(state->npf,
					 (struct mbuf **) &tmp,
					 state->ifp,
					 pf_side);
		if (ret)
			pkts_mask &= ~bit;

		/* set back layer 2 */
		rte_pktmbuf_prepend(pkts[i], sizeof(struct ether_header));
	}

	/* decrement reference of packets which has not been free by npf-dpdk */
	packets_free(pkts, pkts_mask);

	ret = brick_side_forward(s, side, pkts, nb, pkts_mask, errp);
	return ret;
}

static int firewall_init(struct brick *brick,
			 struct brick_config *config,
			 struct switch_error **errp)
{
	/* Global counter for virtual interface. */
	static uint32_t firewall_iface_cnt;

	npf_t *npf;
	struct firewall_state *state = brick_get_state(brick,
						       struct firewall_state);
	/* initialize fast path */
	brick->burst = firewall_burst;
	/* init NPF configuration */
	npf = npf_dpdk_create();
	npf_thread_register(npf);
	state->ifp = npf_dpdk_ifattach(npf, "firewall", firewall_iface_cnt++);
	state->npf = npf;
	state->rules = NULL;

	return 1;
}

static void firewall_destroy(struct brick *brick,
			     struct switch_error **errp) {
	struct firewall_state *state = brick_get_state(brick,
						       struct firewall_state);
	npf_dpdk_ifdetach(state->npf, state->ifp);
	npf_destroy(state->npf);
	firewall_rule_flush(brick);
}

static struct brick_ops firewall_ops = {
	.name		= "firewall",
	.state_size	= sizeof(struct firewall_state),

	.init		= firewall_init,
	.destroy    = firewall_destroy,

	.unlink		= brick_generic_unlink,
};

brick_register(struct firewall_state, &firewall_ops);
