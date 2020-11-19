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

#include <packetgraph/packetgraph.h>
#include "brick-int.h"
#include "utlis/network.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include <errno.h>
#include <rte_mbuf.h>

struct pg_ttl_state {
	struct pg_brick brick;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	uint16_t ttl;
};

static inline void pg_ttl_get_value(struct pg_brick *brick,
		struct rte_mbuf *pkt, struct pg_error **errp)
{
	struct ether_hdr *hdr;
	uint16_t ttl = pg_brick_get_state(brick, struct pg_ttl_state)->ttl;

	hdr = rte_pktmbuf_mtod(pkt, struct ether_hdr *);
	switch (rte_cpu_to_be_16(hdr->ethernet.ether_type)) {
	case ETHER_TYPE_IPv4 :
		ttl = hdr->ipv4.time_to_live;
		break;
	case ETHER_TYPE_IPv6 :
		ttl = hdr->ipv6.hop_limits;
		break;
	default :
		*errp = pg_error_new("error occured in ethernet.ether_type");
		return;
	}
}

void pg_ttl_handle(struct pg_brick *brick, struct rte_mbuf *pkt,
			struct pg_error **errp)
{
	pg_ttl_get_value(brick, pkt, errp);
	struct pg_tap_state *state =
		pg_brick_get_state(brick, struct pg_ttl_state);
	uint16_t ttl = pg_brick_get_state(brick, struct pg_ttl_state)->ttl;

	if (ttl == 0) {
	//drop pkt and ICMP msg
		pg_packets_free(state->pkts, pg_mask_firsts(PG_MAX_PKTS_BURST));
	} else {
		ttl--;
	}
}

static int ttl_init(struct pg_brick *brick,
			    struct pg_brick_config *config,
			    struct pg_error **errp)
{
	struct pg_ttl_state *state;
	struct pg_ttl_config *ttl_config;
	struct rte_mempool *pool = pg_get_mempool();

	state = pg_brick_get_state(brick, struct pg_ttl_state);

	ttl_config =
		(struct pg_ttl_config *) config->brick_config;

	/* pre-allocate packets */
	if (rte_pktmbuf_alloc_bulk(pool, state->pkts,
				PG_MAX_PKTS_BURST) != 0) {
		*errp = pg_error_new("packet allocation failed");
		return -1;
	}
	state->ttl = 64;

	return 0;
}
struct pg_brick *pg_ttl_new(const char *name, struct pg_error **errp)
{
	struct pg_brick_config *config;
	struct pg_brick *ret;

	config = pg_brick_config_new(name, 1, 1, PG_MONOPOLE);
	ret = pg_brick_new("ttl", config, errp);

	pg_brick_config_free(config);
	return ret;
}
static struct pg_brick_ops ttl_ops = {
	.name           = "ttl",
	.state_size     = sizeof(struct pg_ttl_state),

	.init           = ttl_init,

	.unlink         = pg_brick_generic_unlink,
};

pg_brick_register(ttl, &ttl_ops);
