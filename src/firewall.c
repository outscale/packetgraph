/* Copyright 2015 Outscale SAS
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
#include <glib.h>
#include <rte_config.h>
#include <rte_ether.h>
#include <pcap/pcap.h>
#include <endian.h>

#include <packetgraph/packetgraph.h>
#include "utils/bitmask.h"
#include "brick-int.h"
#include "packets.h"
#include "utils/network.h"
#include "utils/mempool.h"
#include "src/npf/npf/dpdk/npf_dpdk.h"

#define FIREWALL_SIDE_TO_NPF(side) \
	((side) == PG_WEST_SIDE ? PFIL_OUT : PFIL_IN)

uint32_t pg_npf_nworkers;

struct ifnet;

struct pg_firewall_state {
	struct pg_brick brick;
	npf_t *npf;
	struct ifnet *ifp;
	GList *rules;
};

struct pg_firewall_config {
	int flags;
};

static uint64_t nb_firewall;

static struct pg_brick_config *firewall_config_new(const char *name, int flags)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_firewall_config *firewall_config =
		g_new0(struct pg_firewall_config, 1);

	firewall_config->flags = flags;
	config->brick_config = (void *) firewall_config;
	return pg_brick_config_init(config, name, 1, 1, PG_DIPOLE);
}

void pg_firewall_gc(struct pg_brick *brick)
{
	struct pg_firewall_state *state;

	state = pg_brick_get_state(brick,
				   struct pg_firewall_state);
	npf_gc(state->npf);
}

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

static inline int firewall_side_to_npf_rule(enum pg_side side)
{
	switch (side) {
	case PG_WEST_SIDE: return NPF_RULE_OUT;
	case PG_EAST_SIDE: return NPF_RULE_IN;
	default: return NPF_RULE_OUT | NPF_RULE_IN;
	}
}

int pg_firewall_rule_add(struct pg_brick *brick, const char *filter,
			 enum pg_side side, int stateful,
			 struct pg_error **errp)
{
	struct pg_firewall_state *state;
	struct nl_rule *rule;
	int options = 0;

	state = pg_brick_get_state(brick, struct pg_firewall_state);
	options |= firewall_side_to_npf_rule(side);
	if (stateful)
		options |= NPF_RULE_STATEFUL;
	rule = npf_rule_create(NULL, NPF_RULE_PASS | options, "firewall");
	g_assert(rule);
	npf_rule_setprio(rule, NPF_PRI_LAST);
	if (filter && firewall_build_pcap_filter(rule, filter)) {
		*errp = pg_error_new("this filter failed to build: %s",
				     filter);
		return -1;
	}
	state->rules = g_list_append(state->rules, rule);
	return 0;
}

void pg_firewall_rule_flush_side(struct pg_brick *brick, enum pg_side side)
{
	struct pg_firewall_state *state;
	GList *it;
	int pf_side = firewall_side_to_npf_rule(side);
	int32_t side_attr;

	state = pg_brick_get_state(brick, struct pg_firewall_state);
	/* clean all rules */
	it = state->rules;
	while (it) {
		side_attr = npf_rule_getattr(it->data) & (NPF_RULE_OUT |
							  NPF_RULE_IN);
		if (side_attr & pf_side) {
			npf_rule_destroy(it->data);
			state->rules = g_list_delete_link(state->rules, it);
			it = state->rules;
			continue;
		}
		it = g_list_next(it);
	}
}

void pg_firewall_rule_flush(struct pg_brick *brick)
{
	struct pg_firewall_state *state;
	GList *it;

	state = pg_brick_get_state(brick, struct pg_firewall_state);
	/* clean all rules */
	it = state->rules;
	while (it) {
		npf_rule_destroy(it->data);
		it = g_list_next(it);
	}
	/* flush list */
	g_list_free(state->rules);
	state->rules = NULL;
}

static int firewall_reload_internal(struct pg_firewall_state *state,
				    struct pg_error **errp)
{
	npf_error_t errinfo;
	struct nl_config *config;
	void *config_build;
	int npf_ret;
	GList *it;

	config = npf_config_create();

	it = state->rules;
	while (it != NULL) {
		npf_rule_insert(config, NULL, it->data);
		it = g_list_next(it);
	}

	config_build = npf_config_build(config);
	npf_ret = npf_load(state->npf, config_build, &errinfo);
	npf_ifmap_attach(state->npf, state->ifp);
	free(config);

	if (npf_ret != 0) {
		*errp = pg_error_new_errno(npf_ret,
					   "NPF failed to load configuration");
		return -1;
	}
	return 0;
}

int pg_firewall_reload(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_firewall_state *state =
		pg_brick_get_state(brick, struct pg_firewall_state);
	return firewall_reload_internal(state, errp);
}

struct pg_brick *pg_firewall_new(const char *name, uint64_t flags,
				 struct pg_error **errp)
{
	struct pg_brick_config *config;
	struct pg_brick *ret;

	config = firewall_config_new(name, flags);
	ret = pg_brick_new("firewall", config, errp);
	pg_brick_config_free(config);
	return ret;
}

static int firewall_burst(struct pg_brick *brick, enum pg_side from,
			  uint16_t edge_index, struct rte_mbuf **pkts,
			  uint64_t pkts_mask,
			  struct pg_error **errp)
{
	struct pg_brick_side *s = &brick->sides[pg_flip_side(from)];
	struct pg_firewall_state *state;
	int pf_side;
	uint64_t it_mask;
	uint64_t bit;
	uint16_t i;
	int ret;
	struct rte_mbuf *tmp;
	uint16_t ether_type;

	state = pg_brick_get_state(brick, struct pg_firewall_state);
	pf_side = FIREWALL_SIDE_TO_NPF(from);

	it_mask = pkts_mask;
	for (; it_mask;) {
		pg_low_bit_iterate_full(it_mask, bit, i);
		tmp = pkts[i];
		ether_type = pg_utils_get_ether_type(tmp);

		/* Firewall only manage IPv4 or IPv6 filtering.
		 * Let non-ip packets (like ARP) pass.
		 */
		if (unlikely(ether_type != PG_BE_ETHER_TYPE_IPv4 &&
			     ether_type != PG_BE_ETHER_TYPE_IPv6)) {
			continue;
		}

		/* NPF only manage layer 3 so we temporaly cut off layer 2.
		 * Note that this trick is not thread safe. To do so, we will
		 * have to clone packets just for filtering and will have to
		 * restroy cloned packets after handling them in NPF.
		 * We directly modify data_off instead of calling
		 * rte_pktmbuf_adj because it's faster
		 */
		tmp->data_off += tmp->l2_len;
		ret = npf_packet_handler(state->npf, (struct mbuf **) &tmp,
					 state->ifp, pf_side);
		pkts[i]->data_off -= pkts[i]->l2_len;
		if (ret)
			pkts_mask &= ~bit;
	}
	if (unlikely(pkts_mask == 0))
		return 0;
	return pg_brick_burst(s->edge.link, from, s->edge.pair_index,
			      pkts, pkts_mask, errp);
}

static int firewall_init(struct pg_brick *brick,
			 struct pg_brick_config *config,
			 struct pg_error **errp)
{
	/* Global counter for virtual interface. */
	static uint32_t firewall_iface_cnt;

	npf_t *npf;
	struct pg_firewall_state *state;
	struct pg_firewall_config *fw_config;
	static int is_dpdk_init;

	state = pg_brick_get_state(brick, struct pg_firewall_state);

	fw_config = (struct pg_firewall_config *) config->brick_config;
	/* initialize fast path */
	brick->burst = firewall_burst;
	/* init NPF configuration */
	if (pg_npf_nworkers == 0 || fw_config->flags & PG_NO_CONN_WORKER)
		fw_config->flags |= NPF_NO_GC;
	if (!nb_firewall) {
		if (unlikely(npf_sysinit(pg_npf_nworkers) < 0)) {
			*errp = pg_error_new("fail during npf initialisation");
			return -1;
		}
		if (!is_dpdk_init) {
			npf_dpdk_init(pg_get_mempool());
			is_dpdk_init = 1;
		}
	}
	npf = npf_dpdk_create(fw_config->flags);
	if (unlikely(!npf)) {
		*errp = pg_error_new("fail to create npf");
		return -1;
	}
	npf_thread_register(npf);
	state->ifp = npf_dpdk_ifattach(npf, "firewall", firewall_iface_cnt++);
	if (!state->ifp) {
		npf_destroy(state->npf);
		*errp = pg_error_new("npf_dpdk_ifattach fail");
		return -1;
	}
	state->npf = npf;
	state->rules = NULL;
	++nb_firewall;
	return firewall_reload_internal(state, errp);
}

static void firewall_destroy(struct pg_brick *brick,
			     struct pg_error **errp)
{
	struct pg_firewall_state *state;

	state = pg_brick_get_state(brick, struct pg_firewall_state);
	pg_firewall_rule_flush(brick);
	npf_dpdk_ifdetach(state->npf, state->ifp);
	npf_thread_unregister(state->npf);
	npf_destroy(state->npf);
	--nb_firewall;
	if (!nb_firewall)
		npf_sysfini();
}

static struct pg_brick_ops firewall_ops = {
	.name		= "firewall",
	.state_size	= sizeof(struct pg_firewall_state),

	.init		= firewall_init,
	.destroy	= firewall_destroy,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(firewall, &firewall_ops);
