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

struct pg_nic_config {
	enum pg_side output;
	char *ifname;
	uint8_t portid;
};

struct pg_nic_state {
	struct pg_brick brick;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	struct rte_mbuf *exit_pkts[PG_MAX_PKTS_BURST];
	uint8_t portid;
	/* side of the physical NIC/PMD */
	enum pg_side output;
};

void pg_nic_start(void)
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

void pg_nic_get_mac(struct pg_brick *nic, struct ether_addr *addr)
{
	struct pg_nic_state *state;

	state = pg_brick_get_state(nic, struct pg_nic_state);
	rte_eth_macaddr_get(state->portid, addr);
}

static struct pg_brick_config *nic_config_new(const char *name,
					      uint32_t west_max,
					      uint32_t east_max,
					      enum pg_side output,
					      const char *ifname,
					      uint8_t portid)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_nic_config *nic_config = g_new0(struct pg_nic_config, 1);

	if (ifname) {
		nic_config->ifname = g_strdup(ifname);
	} else {
		nic_config->ifname = NULL;
		nic_config->portid = portid;
	}
	nic_config->output = output;
	config->brick_config = (void *) nic_config;
	return pg_brick_config_init(config, name, west_max, east_max);
}

void pg_nic_get_stats(struct pg_brick *nic,
		      struct pg_nic_stats *stats)
{
	struct pg_nic_state *state = pg_brick_get_state(nic,
							struct pg_nic_state);

	rte_eth_stats_get(state->portid, (struct rte_eth_stats *)stats);
}

/* The fastpath data function of the nic_brick just forward the bursts */
static int nic_burst(struct pg_brick *brick, enum pg_side from,
		     uint16_t edge_index, struct rte_mbuf **pkts,
		     uint16_t nb_pkts, uint64_t pkts_mask,
		     struct pg_error **errp)
{
	uint16_t count = 0;
	uint16_t pkts_lost;
	struct pg_nic_state *state = pg_brick_get_state(brick,
							struct pg_nic_state);
	struct rte_mbuf **exit_pkts = state->exit_pkts;

	if (state->output == from) {
		*errp = pg_error_new("Bursting on wrong direction");
		return 0;
	}

	count = pg_packets_pack(exit_pkts,
			     pkts, pkts_mask);

	pg_packets_incref(pkts, pkts_mask);
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
		pg_packets_free(exit_pkts, pg_mask_firsts(count) &
				~pg_mask_firsts(pkts_lost));
	}
	return 1;
}

static int nic_poll_forward(struct pg_nic_state *state,
			    struct pg_brick *brick,
			    uint16_t nb_pkts,
			    struct pg_error **errp)
{
	struct pg_brick_side *s = &brick->sides[pg_flip_side(state->output)];
	int ret;
	uint64_t pkts_mask;

	pkts_mask = pg_mask_firsts(nb_pkts);

	ret = pg_brick_side_forward(s, state->output,
				    state->pkts, nb_pkts, pkts_mask, errp);
	pg_packets_free(state->pkts, pkts_mask);
	return ret;
}

static int nic_poll(struct pg_brick *brick, uint16_t *pkts_cnt,
		    struct pg_error **errp)
{
	uint16_t nb_pkts;
	struct pg_nic_state *state =
		pg_brick_get_state(brick, struct pg_nic_state);

	nb_pkts = rte_eth_rx_burst(state->portid, 0,
				   state->pkts, PG_MAX_PKTS_BURST);
	if (!nb_pkts) {
		*pkts_cnt = 0;
		return 1;
	}

	*pkts_cnt = nb_pkts;
	return nic_poll_forward(state, brick, nb_pkts, errp);
}

/* Convert interface name to port id. */
static int ifname_to_portid(char *ifname, struct pg_error **errp)
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
	*errp = pg_error_new("Interface %s not found", ifname);
	return -1;
}

static int nic_init_ports(struct pg_nic_state *state, struct pg_error **errp)
{
	int ret;
	struct rte_mempool *mp = pg_get_mempool();
	static const struct rte_eth_conf port_conf = {
		.rxmode = {
			.split_hdr_size = 0,
			/**< Header Split disabled */
			.header_split   = 0,
			/**< IP checksum offload disabled */
			.hw_ip_checksum = 0,
			/**< VLAN filtering disabled */
			.hw_vlan_filter = 0,
			/**< Jumbo Frame Support disabled */
			.jumbo_frame    = 0,
			/**< CRC stripped by hardware */
			.hw_strip_crc   = 0,
		},
		.txmode = {
			.mq_mode = ETH_MQ_TX_NONE,
		},
	};

	ret = rte_eth_dev_configure(state->portid, 1, 1, &port_conf);
	if (ret < 0) {
		*errp = pg_error_new(
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
		*errp = pg_error_new(
			"Setup failed for port rx queue 0, port %d",
			state->portid);
		return 0;
	}

	ret = rte_eth_tx_queue_setup(state->portid, 0, 64,
				     rte_eth_dev_socket_id(state->portid),
				     NULL);
	if (ret < 0) {
		*errp = pg_error_new(
			"Setup failed for port tx queue 0, port %d",
			state->portid);
		return 0;
	}
	return 1;
}

static int nic_init(struct pg_brick *brick, struct pg_brick_config *config,
		    struct pg_error **errp)
{
	struct pg_nic_state *state;
	struct pg_nic_config *nic_config;
	int ret;

	state = pg_brick_get_state(brick, struct pg_nic_state);

	if (!config) {
		*errp = pg_error_new("config is NULL");
		return 0;
	}
	nic_config = config->brick_config;

	if (!nic_config) {
		*errp = pg_error_new("config->nic is NULL");
		return 0;
	}

	state->output = nic_config->output;
	if (pg_error_is_set(errp))
		return 0;

	/* Setup port id */
	if (nic_config->ifname) {
		state->portid = ifname_to_portid(nic_config->ifname, errp);
		if (pg_error_is_set(errp))
			return 0;
	} else if (nic_config->portid < rte_eth_dev_count()) {
		state->portid = nic_config->portid;
	} else {
		*errp = pg_error_new("Invalid port id %i",
				  nic_config->portid);
		return 0;
	}

	if (!nic_init_ports(state, errp))
		return 0;

	ret = rte_eth_dev_start(state->portid);
	if (ret < 0) {
		*errp = pg_error_new("Device failed to start on port %d",
				     state->portid);
		return ret;
	}
	rte_eth_promiscuous_enable(state->portid);


	brick->burst = nic_burst;
	brick->poll = nic_poll;

	return 1;
}

static void nic_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_nic_state *state =
		pg_brick_get_state(brick, struct pg_nic_state);
	rte_eth_xstats_reset(state->portid);
	rte_eth_dev_stop(state->portid);
}

struct pg_brick *pg_nic_new(const char *name,
			    uint32_t west_max,
			    uint32_t east_max,
			    enum pg_side output,
			    const char *ifname,
			    struct pg_error **errp)
{
	struct pg_brick_config *config = nic_config_new(name, west_max,
							east_max, output,
							ifname, 0);

	struct pg_brick *ret = pg_brick_new("nic", config, errp);

	pg_brick_config_free(config);
	return ret;
}

struct pg_brick *pg_nic_new_by_id(const char *name,
				  uint32_t west_max,
				  uint32_t east_max,
				  enum pg_side output,
				  uint8_t portid,
				  struct pg_error **errp)
{
	struct pg_brick_config *config = nic_config_new(name,
							west_max, east_max,
							output, NULL,
							portid);

	struct pg_brick *ret = pg_brick_new("nic", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static struct pg_brick_ops nic_ops = {
	.name		= "nic",
	.state_size	= sizeof(struct pg_nic_state),

	.init		= nic_init,
	.destroy	= nic_destroy,

	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(nic, &nic_ops);
