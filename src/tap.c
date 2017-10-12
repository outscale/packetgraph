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
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <rte_config.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_memcpy.h>

#include <packetgraph/packetgraph.h>
#include "brick-int.h"
#include "packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "utils/network.h"

struct pg_tap_config {
	char ifname[IFNAMSIZ];
};

struct pg_tap_state {
	struct pg_brick brick;
	int tap_fd;
	struct ifreq ifr;
	struct rte_mbuf *pkts[PG_MAX_PKTS_BURST];
	/* side of the kernel interface */
	enum pg_side output;
};

const char *pg_tap_ifname(struct pg_brick *brick)
{
	struct pg_tap_state *state =
		pg_brick_get_state(brick, struct pg_tap_state);

	return state->ifr.ifr_name;
}

int pg_tap_get_mac(struct pg_brick *tap, struct ether_addr *addr)
{
	struct ifreq i;
	int fd;

	strcpy(i.ifr_name, pg_tap_ifname(tap));
	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (ioctl(fd, SIOCGIFHWADDR, &i) < 0)
		return -1;
	rte_memcpy(addr, i.ifr_addr.sa_data, 6);
	return 0;
}

static struct pg_brick_config *tap_config_new(const char *name,
					      const char *ifname)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_tap_config *tap_config = g_new0(struct pg_tap_config, 1);

	if (ifname)
		strncpy(tap_config->ifname, ifname, IFNAMSIZ);
	config->brick_config = (void *) tap_config;
	return pg_brick_config_init(config, name, 1, 1, PG_MONOPOLE);
}

static int tap_burst(struct pg_brick *brick, enum pg_side from,
		     uint16_t edge_index, struct rte_mbuf **pkts,
		     uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_tap_state *state = pg_brick_get_state(brick,
							struct pg_tap_state);
	uint64_t it_mask;
	uint16_t i;
	int fd = state->tap_fd;

	it_mask = pkts_mask;
	for (; it_mask;) {
		pg_low_bit_iterate(it_mask, i);
		struct rte_mbuf *packet = pkts[i];
		char *data = rte_pktmbuf_mtod(packet, char *);
		int len = rte_pktmbuf_pkt_len(packet);

		/* write data. */
		if (unlikely(write(fd, data, len) < 0)) {
			*errp = pg_error_new("%s", strerror(errno));
			return -1;
		}
	}

#ifdef PG_TAP_BENCH
	struct pg_brick_side *side = &brick->side;

	if (side->burst_count_cb != NULL) {
		side->burst_count_cb(side->burst_count_private_data,
				     pg_mask_count(pkts_mask));
	}
#endif /* #ifdef PG_TAP_BENCH */
	return 0;
}

static int tap_poll(struct pg_brick *brick, uint16_t *pkts_cnt,
		    struct pg_error **errp)
{
	int nb_pkts;
	uint64_t pkts_mask;
	int ret;
	struct pg_tap_state *state =
		pg_brick_get_state(brick, struct pg_tap_state);
	int fd = state->tap_fd;
	struct pg_brick_side *s = &brick->side;
	struct rte_mbuf **pkts = state->pkts;

	if (unlikely(s->edge.link == NULL))
		return 0;

	for (nb_pkts = 0; nb_pkts < PG_MAX_PKTS_BURST; nb_pkts++) {
		struct rte_mbuf *packet = pkts[nb_pkts];

		rte_pktmbuf_reset(packet);
		char *data = rte_pktmbuf_mtod(packet, char *);
		int read_size = read(fd, data, PG_MBUF_SIZE);

		static_assert(PG_MBUF_SIZE > 1500,
			      "reminder: should fix code below");

		if (read_size < 0) {
			if (likely(errno == EAGAIN))
				break;
			*errp = pg_error_new("%s", strerror(errno));
			return -1;
		}
		/* ignore potential truncated packets */
		if (unlikely(read_size == PG_MBUF_SIZE)) {
			nb_pkts--;
			continue;
		}

		rte_pktmbuf_append(packet, read_size);
		pg_utils_guess_metadata(packet);
	}

	*pkts_cnt = nb_pkts;
	if (nb_pkts == 0)
		return 0;
	pkts_mask = pg_mask_firsts(nb_pkts);
	ret = pg_brick_burst(s->edge.link, state->output,
			     s->edge.pair_index,
			     state->pkts, pkts_mask, errp);
	return ret;
}

static int tap_init(struct pg_brick *brick, struct pg_brick_config *config,
		    struct pg_error **errp)
{
	struct pg_tap_state *state;
	struct pg_tap_config *tap_config;
	struct rte_mempool *pool = pg_get_mempool();
	int tap_fd;

	tap_fd = open("/dev/net/tun", O_NONBLOCK | O_RDWR);
	if (tap_fd < 0) {
		*errp = pg_error_new("Cannot open /dev/net/tun");
		return -1;
	}

	state = pg_brick_get_state(brick, struct pg_tap_state);
	memset(&state->ifr, 0, sizeof(struct ifreq));
	tap_config = config->brick_config;
	strncpy(state->ifr.ifr_name, tap_config->ifname, IFNAMSIZ);
	state->ifr.ifr_flags = IFF_NO_PI | IFF_TAP;

	if (ioctl(tap_fd, TUNSETIFF, (void *) &state->ifr) < 0) {
		*errp = pg_error_new("ioctl error (TUNSETIFF)");
		goto error;
	}

	if (ioctl(tap_fd, TUNSETPERSIST, 0) < 0) {
		*errp = pg_error_new("ioctl error (TUNSETPERSIST)");
		goto error;
	}

	/* pre-allocate packets */
	if (rte_pktmbuf_alloc_bulk(pool, state->pkts,
				   PG_MAX_PKTS_BURST) != 0) {
		*errp = pg_error_new("packet allocation failed");
		goto error;
	}

	state->tap_fd = tap_fd;
	brick->burst = tap_burst;
	brick->poll = tap_poll;
	return 0;
error:
	close(tap_fd);
	return -1;
}

static void tap_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_tap_state *state =
		pg_brick_get_state(brick, struct pg_tap_state);

	close(state->tap_fd);
	pg_packets_free(state->pkts, pg_mask_firsts(PG_MAX_PKTS_BURST));
}

struct pg_brick *pg_tap_new(const char *name,
			    const char *ifname,
			    struct pg_error **errp)
{
	struct pg_brick_config *config = tap_config_new(name, ifname);
	struct pg_brick *ret = pg_brick_new("tap", config, errp);

	pg_brick_config_free(config);
	return ret;
}

static void tap_link(struct pg_brick *brick, enum pg_side side, int edge)
{
	struct pg_tap_state *state =
		pg_brick_get_state(brick, struct pg_tap_state);
	/*
	 * We flip the side, because we don't want to flip side when
	 * we burst
	 */
	state->output = pg_flip_side(side);
}

static enum pg_side tap_get_side(struct pg_brick *brick)
{
	struct pg_tap_state *state = pg_brick_get_state(brick,
							struct pg_tap_state);
	return pg_flip_side(state->output);
}

static struct pg_brick_ops tap_ops = {
	.name		= "tap",
	.state_size	= sizeof(struct pg_tap_state),

	.init		= tap_init,
	.destroy	= tap_destroy,
	.link_notify	= tap_link,
	.get_side	= tap_get_side,
	.unlink		= pg_brick_generic_unlink,
};

pg_brick_register(tap, &tap_ops);
