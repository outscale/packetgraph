/* Copyright 2014 Outscale SAS
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

#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>

#include <rte_cycles.h>
#include <net/if.h>

#include <packetgraph/packetgraph.h>
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/network.h"
#include "nic-int.h"

#define NIC_ARGS_MAX_SIZE 1024
#define TCP_PROTOCOL_NUMBER 6
#define UDP_PROTOCOL_NUMBER 17

struct pg_nic_config {
	char ifname[NIC_ARGS_MAX_SIZE];
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

struct headers_eth_ipv4_l4 {
	struct ether_hdr ethernet;
	struct ipv4_hdr ipv4; /* define in rte_ip.h */
	union {
		struct udp_hdr udp;
		struct tcp_hdr tcp;
	};
} __attribute__((__packed__));


void pg_nic_get_mac(struct pg_brick *nic, struct ether_addr *addr)
{
	struct pg_nic_state *state;

	state = pg_brick_get_state(nic, struct pg_nic_state);
	rte_eth_macaddr_get(state->portid, addr);
}

void pg_nic_capabilities(struct pg_brick *nic, uint32_t *rx, uint32_t *tx)
{
	struct pg_nic_state *state;
	struct rte_eth_dev_info dev_info;

	state = pg_brick_get_state(nic, struct pg_nic_state);
	rte_eth_dev_info_get(state->portid, &dev_info);

	*rx = dev_info.rx_offload_capa;
	*tx = dev_info.tx_offload_capa;
}

static struct pg_brick_config *nic_config_new(const char *name,
					      const char *ifname,
					      uint8_t portid)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_nic_config *nic_config = g_new0(struct pg_nic_config, 1);

	if (ifname) {
		g_strlcpy(nic_config->ifname, ifname, NIC_ARGS_MAX_SIZE);
	} else {
		nic_config->ifname[0] = '\0';
		nic_config->portid = portid;
	}
	config->brick_config = (void *) nic_config;
	return pg_brick_config_init(config, name, 1, 1, PG_MONOPOLE);
}

static uint64_t rx_bytes(struct pg_brick *brick)
{
	struct pg_nic_state *state;
	struct rte_eth_stats tmp;

	state = pg_brick_get_state(brick, struct pg_nic_state);
	rte_eth_stats_get(state->portid, &tmp);
	return tmp.ibytes;
}

static uint64_t tx_bytes(struct pg_brick *brick)
{
	struct pg_nic_state *state;
	struct rte_eth_stats tmp;

	state = pg_brick_get_state(brick, struct pg_nic_state);
	rte_eth_stats_get(state->portid, &tmp);
	return tmp.obytes;
}

/* The fastpath data function of the nic_brick just forward the bursts */
static int nic_burst(struct pg_brick *brick, enum pg_side from,
		     uint16_t edge_index, struct rte_mbuf **pkts,
		     uint64_t pkts_mask,
		     struct pg_error **errp)
{
	uint16_t count = 0;
	uint16_t pkts_bursted;
	struct pg_nic_state *state = pg_brick_get_state(brick,
							struct pg_nic_state);
	struct rte_mbuf **exit_pkts = state->exit_pkts;

	count = pg_packets_pack(exit_pkts,
				pkts, pkts_mask);

	pg_packets_incref(pkts, pkts_mask);
	rte_eth_tx_prepare(state->portid, 0, exit_pkts, count);
#ifndef PG_NIC_STUB
	pkts_bursted = rte_eth_tx_burst(state->portid, 0,
					exit_pkts,
					count);
#else
	if (count > max_pkts)
		pkts_bursted = rte_eth_tx_burst(state->portid, 0,
						exit_pkts, max_pkts);
	else
		pkts_bursted = rte_eth_tx_burst(state->portid, 0,
						exit_pkts, count);
#endif /* #ifndef PG_NIC_STUB */

#ifdef PG_NIC_BENCH
	struct pg_brick_side *side = &brick->side;

	if (side->burst_count_cb != NULL) {
		side->burst_count_cb(side->burst_count_private_data,
				     pkts_bursted);
	}
#endif /* #ifdef PG_NIC_BENCH */

	if (unlikely(pkts_bursted < count)) {
		pg_packets_free(exit_pkts, pg_mask_firsts(count) &
				~pg_mask_firsts(pkts_bursted));
	}
	return 0;
}

static int nic_burst_no_offload(struct pg_brick *brick, enum pg_side from,
				    uint16_t edge_index,
				    struct rte_mbuf **pkts,
				    uint64_t pkts_mask,
				    struct pg_error **errp)
{
	uint64_t mask = pkts_mask;

	for (; mask;) {
		uint16_t i;
		struct rte_mbuf *pkt;
		struct headers_eth_ipv4_l4 *hdr;

		pg_low_bit_iterate(mask, i);
		pkt = pkts[i];
		if (pkt->ol_flags & (PKT_TX_UDP_CKSUM | PKT_TX_TCP_CKSUM)) {
			uint16_t ipv4_csum;
			uint8_t *hdr_byte;
			uint8_t next_proto_id;

			hdr_byte = rte_pktmbuf_mtod(pkt, uint8_t *);
			hdr_byte += (pkt->l2_len - sizeof(struct ether_hdr));
			hdr = (void *)hdr_byte;

			next_proto_id = hdr->ipv4.next_proto_id;
			if (rte_cpu_to_be_16(hdr->ethernet.ether_type) !=
			    ETHER_TYPE_IPv4 ||
			    (next_proto_id != TCP_PROTOCOL_NUMBER &&
			     next_proto_id != UDP_PROTOCOL_NUMBER)) {
				continue;
			}
			ipv4_csum = hdr->ipv4.hdr_checksum;
			hdr->ipv4.hdr_checksum = 0;
			if (next_proto_id == TCP_PROTOCOL_NUMBER) {
				hdr->tcp.cksum = 0;
				hdr->tcp.cksum =
					rte_ipv4_udptcp_cksum(&hdr->ipv4,
							      &hdr->tcp);
			} else {
				hdr->udp.dgram_cksum = 0;
				hdr->udp.dgram_cksum =
					rte_ipv4_udptcp_cksum(&hdr->ipv4,
							      &hdr->udp);
			}
			hdr->ipv4.hdr_checksum = ipv4_csum;
		}
	}
	return nic_burst(brick, from, edge_index, pkts, pkts_mask, errp);
}

static int nic_poll_forward(struct pg_nic_state *state,
			    struct pg_brick *brick,
			    uint16_t nb_pkts,
			    struct pg_error **errp)
{
	struct pg_brick_side *s = &brick->side;
	int ret;
	uint64_t pkts_mask;

	if (unlikely(s->edge.link == NULL))
		return 0;

	pkts_mask = pg_mask_firsts(nb_pkts);

	ret = pg_brick_burst(s->edge.link, state->output,
			     s->edge.pair_index,
			     state->pkts, pkts_mask, errp);

	pg_packets_free(state->pkts, pkts_mask);
	return ret;
}

static int nic_poll(struct pg_brick *brick, uint16_t *pkts_cnt,
		    struct pg_error **errp)
{
	uint16_t nb_pkts;
	struct pg_nic_state *state =
		pg_brick_get_state(brick, struct pg_nic_state);
	struct rte_mbuf **pkts = state->pkts;

	nb_pkts = rte_eth_rx_burst(state->portid, 0,
				   state->pkts, PG_MAX_PKTS_BURST);
	if (!nb_pkts) {
		*pkts_cnt = 0;
		return 0;
	}


	for (int i = 0; i < nb_pkts; i++)
		pg_utils_guess_metadata(pkts[i]);
	*pkts_cnt = nb_pkts;
	return nic_poll_forward(state, brick, nb_pkts, errp);
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
	static const struct rte_eth_txconf tx_conf = {
		.txq_flags = 0,
	};

	ret = rte_eth_dev_configure(state->portid, 1, 1, &port_conf);
	if (ret < 0) {
		*errp = pg_error_new(
			"Port configuration %u failed (may already be in use)",
			state->portid);
		return -1;
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
		return -1;
	}

	ret = rte_eth_tx_queue_setup(state->portid, 0, 64,
				     rte_eth_dev_socket_id(state->portid),
				     &tx_conf);
	if (ret < 0) {
		*errp = pg_error_new(
			"Setup failed for port tx queue 0, port %d",
			state->portid);
		return -1;
	}
	return 0;
}

static int nic_init(struct pg_brick *brick, struct pg_brick_config *config,
		    struct pg_error **errp)
{
	struct pg_nic_state *state;
	struct pg_nic_config *nic_config;
	struct rte_eth_txq_info qinfo;
	int ret;

	state = pg_brick_get_state(brick, struct pg_nic_state);
	nic_config = config->brick_config;

	/* Setup port id */
	if (nic_config->ifname[0]) {
		gchar *tmp = g_strdup_printf("%s", nic_config->ifname);

		if (rte_eth_dev_attach(tmp, &state->portid) < 0) {
			*errp = pg_error_new("Invalid parameter %s",
					     nic_config->ifname);
		}
		g_free(tmp);
		if (pg_error_is_set(errp))
			return -1;
	} else if (nic_config->portid < rte_eth_dev_count()) {
		state->portid = nic_config->portid;
	} else {
		*errp = pg_error_new("Invalid port id %i",
				  nic_config->portid);
		return -1;
	}

	if (nic_init_ports(state, errp) < 0)
		return -1;

	ret = rte_eth_dev_start(state->portid);
	if (ret < 0) {
		*errp = pg_error_new("Device failed to start on port %d",
				     state->portid);
		return -1;
	}
	rte_eth_promiscuous_enable(state->portid);

	/* check if nic supports offloading */
	if (rte_eth_tx_queue_info_get(state->portid, 0, &qinfo) == 0 &&
	    ((qinfo.conf.txq_flags & ETH_TXQ_FLAGS_NOXSUMUDP) == 0 ||
	     (qinfo.conf.txq_flags & ETH_TXQ_FLAGS_NOXSUMTCP) == 0)) {
		brick->burst = nic_burst;
	} else {
		brick->burst = nic_burst_no_offload;
	}
	brick->poll = nic_poll;

	return 0;
}

static void nic_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_nic_state *state =
		pg_brick_get_state(brick, struct pg_nic_state);
	rte_eth_xstats_reset(state->portid);
	rte_eth_dev_stop(state->portid);
	rte_eth_dev_close(state->portid);
}

struct pg_brick *pg_nic_new(const char *name,
			    const char *ifname,
			    struct pg_error **errp)
{
	struct pg_brick_config *config = nic_config_new(name, ifname, 0);

	struct pg_brick *ret = pg_brick_new("nic", config, errp);

	pg_brick_config_free(config);
	return ret;
}

int pg_nic_set_mtu(struct pg_brick *brick, uint16_t mtu,
		   struct pg_error **errp)
{
	struct pg_nic_state *state = pg_brick_get_state(brick,
							struct pg_nic_state);
	int ret = rte_eth_dev_set_mtu(state->portid, mtu);

	if (ret < 0) {
		*errp = pg_error_new_errno(-ret,
					   "cannot set MTU");
		return -1;
	}
	return 0;
}

int pg_nic_get_mtu(struct pg_brick *brick, uint16_t *mtu,
		   struct pg_error **errp)
{
	struct pg_nic_state *state = pg_brick_get_state(brick,
							struct pg_nic_state);
	int ret = rte_eth_dev_get_mtu(state->portid, mtu);

	if (ret < 0) {
		*errp = pg_error_new_errno(-ret,
					   "cannot get MTU");
		return -1;
	}
	return 0;
}

struct pg_brick *pg_nic_new_by_id(const char *name,
				  uint8_t portid,
				  struct pg_error **errp)
{
	struct pg_brick_config *config = nic_config_new(name,
							NULL,
							portid);

	struct pg_brick *ret = pg_brick_new("nic", config, errp);

	pg_brick_config_free(config);
	return ret;
}

int pg_nic_port_count(void)
{
	return rte_eth_dev_count();
}

static void nic_link(struct pg_brick *brick, enum pg_side side, int edge)
{
	struct pg_nic_state *state = pg_brick_get_state(brick,
							struct pg_nic_state);
	/*
	 * We flip the side, because we don't want to flip side when
	 * we burst
	 */
	state->output = pg_flip_side(side);
}

static enum pg_side nic_get_side(struct pg_brick *brick)
{
	struct pg_nic_state *state = pg_brick_get_state(brick,
							struct pg_nic_state);
	return pg_flip_side(state->output);
}

static struct pg_brick_ops nic_ops = {
	.name		= "nic",
	.state_size	= sizeof(struct pg_nic_state),

	.init		= nic_init,
	.destroy	= nic_destroy,
	.link_notify	= nic_link,
	.get_side	= nic_get_side,
	.unlink		= pg_brick_generic_unlink,
	.rx_bytes	= rx_bytes,
	.tx_bytes	= tx_bytes,
};

pg_brick_register(nic, &nic_ops);

#undef NIC_ARGS_MAX_SIZE
#undef TCP_PROTOCOL_NUMBER
#undef UDP_PROTOCOL_NUMBER
