/* Copyright 2014 Outscale SAS
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

#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <net/if.h>

#include <packetgraph/common.h>
#include <packetgraph/brick.h>
#include <packetgraph/packets.h>
#include <packetgraph/utils/mempool.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/nic.h>
#include "nic.h"

struct nic_config {
	enum side output;
	char *ifname;
	uint8_t portid;
};

struct nic_state {
	struct brick brick;
	struct rte_mbuf *pkts[MAX_PKTS_BURST];
	struct rte_mbuf *exit_pkts[MAX_PKTS_BURST];
	uint8_t portid;
	/* side of the physical NIC/PMD */
	enum side output;
};

void nic_start(void)
{
	devinitfn_pmd_pcap_drv();

#ifdef RTE_LIBRTE_PMD_RING
	devinitfn_pmd_ring_drv();
#endif

#ifdef RTE_LIBRTE_IGB_PMD
	devinitfn_pmd_igb_drv();
#endif

#ifdef RTE_LIBRTE_IXGBE_PMD
	devinitfn_rte_ixgbe_driver();
#endif
}

static struct brick_config *nic_config_new(const char *name, uint32_t west_max,
				    uint32_t east_max,
				    enum side output,
				    const char *ifname,
				    uint8_t portid)
{
	struct brick_config *config = g_new0(struct brick_config, 1);
	struct nic_config *nic_config = g_new0(struct nic_config, 1);

	if (ifname) {
		nic_config->ifname = g_strdup(ifname);
	} else {
		nic_config->ifname = NULL;
		nic_config->portid = portid;
	}
	nic_config->output = output;
	config->brick_config = (void *) nic_config;
	return brick_config_init(config, name, west_max, east_max);
}

void nic_get_stats(struct brick *nic,
		   struct nic_stats *stats)
{
	struct nic_state *state = brick_get_state(nic, struct nic_state);

	rte_eth_stats_get(state->portid, (struct rte_eth_stats *)stats);
}

/* The fastpath data function of the nic_brick just forward the bursts */
static int nic_burst(struct brick *brick, enum side from, uint16_t edge_index,
		     struct rte_mbuf **pkts, uint16_t nb_pkts,
		     uint64_t pkts_mask, struct switch_error **errp)
{
	uint16_t count = 0;
	uint16_t pkts_lost;
	struct nic_state *state = brick_get_state(brick, struct nic_state);
	struct rte_mbuf **exit_pkts = state->exit_pkts;

	if (state->output == from) {
		*errp = error_new("Bursting on wrong direction");
		return 0;
	}

	count = packets_pack(exit_pkts,
			     pkts, pkts_mask);

	packets_incref(pkts, pkts_mask);
#ifndef _BRICKS_BRICK_NIC_STUB_H_
	pkts_lost = rte_eth_tx_burst(state->portid, 0,
				     exit_pkts,
				     count);
#else
	if (nb_pkts > max_pkts)
		pkts_lost = rte_eth_tx_burst(state->portid, 0,
					     exit_pkts, max_pkts);
	else
		pkts_lost = rte_eth_tx_burst(state->portid, 0,
					     exit_pkts, nb_pkts);
#endif /* #ifndef _BRICKS_BRICK_NIC_STUB_H_ */

	if (unlikely(pkts_lost < count)) {
		packets_free(exit_pkts, mask_firsts(count) &
			     ~mask_firsts(pkts_lost));
	}
	return 1;
}

static int nic_poll_forward(struct nic_state *state,
			    struct brick *brick,
			    uint16_t nb_pkts,
			    struct switch_error **errp)
{
	struct brick_side *s = &brick->sides[flip_side(state->output)];
	int ret;
	uint64_t pkts_mask;

	pkts_mask = mask_firsts(nb_pkts);

	ret = brick_side_forward(s, state->output,
				 state->pkts, nb_pkts, pkts_mask, errp);
	packets_free(state->pkts, pkts_mask);
	return ret;
}

static int nic_poll(struct brick *brick, uint16_t *pkts_cnt,
		    struct switch_error **errp)
{
	uint16_t nb_pkts;
	struct nic_state *state =
		brick_get_state(brick, struct nic_state);

	nb_pkts = rte_eth_rx_burst(state->portid, 0,
				   state->pkts, MAX_PKTS_BURST);
	if (!nb_pkts) {
		*pkts_cnt = 0;
		return 1;
	}

	*pkts_cnt = nb_pkts;
	return nic_poll_forward(state, brick, nb_pkts, errp);
}

/* Convert interface name to port id. */
static int ifname_to_portid(char *ifname, struct switch_error **errp)
{
	uint8_t port_id = -1;
	uint8_t port_id_max;

	port_id_max = rte_eth_dev_count();
	for (port_id = 0; port_id < port_id_max; port_id++) {
		struct rte_eth_dev_info dev_info;
		char tmp[IF_NAMESIZE];

		rte_eth_dev_info_get(port_id, &dev_info);

		if (if_indextoname(dev_info.if_index, tmp) == NULL) {
			/* try to get name from DPDK */
			struct rte_eth_dev *dev = &rte_eth_devices[port_id];

			g_strlcpy(tmp, dev->data->name, IF_NAMESIZE);
		}
		if (g_str_equal(ifname, tmp))
			return port_id;
	}
	*errp = error_new("Interface %s not found", ifname);
	return -1;
}

static int nic_init_ports(struct nic_state *state, struct switch_error **errp)
{
	int ret;
	struct rte_mempool *mp = get_mempool();
	static const struct rte_eth_conf port_conf = {
		.rxmode = {
			.split_hdr_size = 0,
			.header_split   = 0, /**< Header Split disabled */
			.hw_ip_checksum = 0, /**< IP checksum offload disabled */
			.hw_vlan_filter = 0, /**< VLAN filtering disabled */
			.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
			.hw_strip_crc   = 0, /**< CRC stripped by hardware */
		},
		.txmode = {
			.mq_mode = ETH_MQ_TX_NONE,
		},
	};

	ret = rte_eth_dev_configure(state->portid, 1, 1, &port_conf);
	if (ret < 0) {
		*errp = error_new(
			"Port configuration %u failed (may already be in use)",
			state->portid);
		return 0;
	}

	/* Setup queues */
	ret = rte_eth_rx_queue_setup(state->portid, 0, 64,
				     rte_eth_dev_socket_id(state->portid),
				     NULL,
				     mp);
	if (ret < 0) {
		*errp = error_new("Setup failed for port rx queue 0, port %d",
				  state->portid);
		return 0;
	}

	ret = rte_eth_tx_queue_setup(state->portid, 0, 64,
				     rte_eth_dev_socket_id(state->portid),
				     NULL);
	if (ret < 0) {
		*errp = error_new("Setup failed for port tx queue 0, port %d",
				  state->portid);
		return 0;
	}
	return 1;
}

static int nic_init(struct brick *brick, struct brick_config *config,
		    struct switch_error **errp)
{
	struct nic_state *state = brick_get_state(brick, struct nic_state);
	struct nic_config *nic_config;
	int ret;

	if (!config) {
		*errp = error_new("config is NULL");
		return 0;
	}
	nic_config = config->nic;

	if (!nic_config) {
		*errp = error_new("config->nic is NULL");
		return 0;
	}

	state->output = nic_config->output;
	if (error_is_set(errp))
		return 0;

	/* Setup port id */
	if (nic_config->ifname) {
		state->portid = ifname_to_portid(nic_config->ifname, errp);
		if (error_is_set(errp))
			return 0;
	} else if (nic_config->portid < rte_eth_dev_count()) {
		state->portid = nic_config->portid;
	} else {
		*errp = error_new("Invalid port id %i",
				  nic_config->portid);
		return 0;
	}

	if (!nic_init_ports(state, errp))
		return 0;

	ret = rte_eth_dev_start(state->portid);
	if (ret < 0) {
		*errp = error_new("Device failed to start on port %d",
				  state->portid);
		return ret;
	}
	rte_eth_promiscuous_enable(state->portid);


	brick->burst = nic_burst;
	brick->poll = nic_poll;

	return 1;
}

static void nic_destroy(struct brick *brick, struct switch_error **errp)
{
	struct nic_state *state =
		brick_get_state(brick, struct nic_state);
	rte_eth_xstats_reset(state->portid);
	rte_eth_dev_stop(state->portid);
}

struct brick *nic_new(const char *name,
		      uint32_t west_max,
		      uint32_t east_max,
		      enum side output,
		      const char *ifname,
		      struct switch_error **errp)
{
	struct brick_config *config = nic_config_new(name, west_max,
						     east_max, output, ifname,
						     0);

	struct brick *ret = brick_new("nic", config, errp);

	brick_config_free(config);
	return ret;
}

struct brick *nic_new_by_id(const char *name,
		      uint32_t west_max,
		      uint32_t east_max,
		      enum side output,
		      uint8_t portid,
		      struct switch_error **errp)
{
	struct brick_config *config = nic_config_new(name,
						     west_max, east_max,
						     output, NULL,
						     portid);

	struct brick *ret = brick_new("nic", config, errp);

	brick_config_free(config);
	return ret;
}

static struct brick_ops nic_ops = {
	.name		= "nic",
	.state_size	= sizeof(struct nic_state),

	.init		= nic_init,
	.destroy	= nic_destroy,

	.unlink		= brick_generic_unlink,
};

brick_register(nic, &nic_ops);
