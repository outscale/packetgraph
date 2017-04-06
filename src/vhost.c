/* Copyright 2015 Nodalink EURL
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <rte_config.h>
#include <linux/virtio_net.h>
#include <linux/virtio_ring.h>
#include <rte_virtio_net.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>

#include <packetgraph/packetgraph.h>
#include "brick-int.h"
#include "packets.h"
#include "utils/bitmask.h"
#include "utils/mempool.h"
#include "utils/network.h"

#define MAX_BURST 32

struct pg_vhost_config {
	enum pg_side output;
	uint64_t flags;
};

struct pg_vhost_socket {
	char *path;			/* path of the socket */
	struct pg_vhost_state *state;	/* NULL if the socket is free */

	LIST_ENTRY(pg_vhost_socket) socket_list; /* sockets list */
};

struct pg_vhost_state {
	struct pg_brick brick;
	enum pg_side output;
	struct pg_vhost_socket *socket;
	rte_atomic32_t allow_queuing;
	int vid;
	struct rte_mbuf *in[PG_MAX_PKTS_BURST];
	struct rte_mbuf *out[PG_MAX_PKTS_BURST];
	rte_atomic64_t tx_bytes; /* TX: [vhost] --> VM */
	rte_atomic64_t rx_bytes; /* RX: [vhost] <-- VM */
};

/* head of the socket list */
static LIST_HEAD(socket_list, pg_vhost_socket) sockets;
static char *sockets_path;
static int vhost_start_ok;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t vhost_session_thread;

static struct pg_brick_config *vhost_config_new(const char *name,
						uint64_t flags)
{
	struct pg_brick_config *config = g_new0(struct pg_brick_config, 1);
	struct pg_vhost_config *vhost_config = g_new0(struct pg_vhost_config,
						      1);

	config->brick_config = (void *) vhost_config;
	vhost_config->flags = flags;
	return pg_brick_config_init(config, name, 1, 1, PG_MONOPOLE);
}

static int vhost_burst(struct pg_brick *brick, enum pg_side from,
		       uint16_t edge_index, struct rte_mbuf **pkts,
		       uint64_t pkts_mask, struct pg_error **errp)
{
	struct pg_vhost_state *state;
	int virtio_net;
	uint16_t pkts_count;
	uint16_t bursted_pkts;
	uint64_t tx_bytes = 0;

	state = pg_brick_get_state(brick, struct pg_vhost_state);

	/* Try lock */
	if (unlikely(!rte_atomic32_test_and_set(&state->allow_queuing)))
		return 0;

	virtio_net = state->vid;
	pkts_count = pg_packets_pack(state->out, pkts, pkts_mask);
	bursted_pkts = rte_vhost_enqueue_burst(virtio_net,
					       VIRTIO_RXQ,
					       state->out,
					       pkts_count);
	rte_atomic32_clear(&state->allow_queuing);

	/* count tx bytes: burst is packed so we can directly iterate */
	for (int i = 0; i < bursted_pkts; i++)
		tx_bytes += rte_pktmbuf_pkt_len(pkts[i]);
	rte_atomic64_add(&state->tx_bytes, tx_bytes);

#ifdef PG_VHOST_BENCH
	struct pg_brick_side *side = &brick->side;

	if (side->burst_count_cb != NULL)
		side->burst_count_cb(side->burst_count_private_data,
				     bursted_pkts);
#endif /* #ifdef PG_VHOST_BENCH */
	return 0;
}

#define TCP_PROTOCOL_NUMBER 6
#define UDP_PROTOCOL_NUMBER 17

static int vhost_poll(struct pg_brick *brick, uint16_t *pkts_cnt,
		      struct pg_error **errp)
{
	struct pg_vhost_state *state =
		pg_brick_get_state(brick, struct pg_vhost_state);
	struct pg_brick_side *s = &brick->side;
	struct rte_mempool *mp = pg_get_mempool();
	int virtio_net;
	uint64_t pkts_mask;
	uint16_t count;
	int ret;
	struct rte_mbuf **in = state->in;
	uint64_t rx_bytes = 0;

	*pkts_cnt = 0;
	/* Try lock */
	if (unlikely(!rte_atomic32_test_and_set(&state->allow_queuing)))
		return 0;

	rte_atomic32_set(&state->allow_queuing, 0);
	virtio_net = state->vid;
	*pkts_cnt = 0;

	count = rte_vhost_dequeue_burst(virtio_net, VIRTIO_TXQ, mp, in,
					MAX_BURST);
	*pkts_cnt = count;

	rte_atomic32_clear(&state->allow_queuing);
	if (!count)
		return 0;

	for (int i = 0; i < count; i += 1) {
		rx_bytes += rte_pktmbuf_pkt_len(in[i]);
		pg_utils_guess_metadata(in[i]);
	}

	rte_atomic64_add(&state->rx_bytes, rx_bytes);

	pkts_mask = pg_mask_firsts(count);
	ret = pg_brick_burst(s->edge.link, state->output, s->edge.pair_index,
			     in, pkts_mask, errp);
	pg_packets_free(in, pkts_mask);
	return ret;
}

#undef TCP_PROTOCOL_NUMBER
#undef UDP_PROTOCOL_NUMBER

#ifndef RTE_VHOST_USER_CLIENT
#define RTE_VHOST_USER_CLIENT 0
#endif

#ifndef RTE_VHOST_USER_NO_RECONNECT
#define RTE_VHOST_USER_NO_RECONNECT 0
#endif

#ifndef RTE_VHOST_USER_DEQUEUE_ZERO_COPY
#define RTE_VHOST_USER_DEQUEUE_ZERO_COPY 0
#endif

static void vhost_create_socket(struct pg_vhost_state *state, uint64_t flags,
				struct pg_error **errp)
{
	struct pg_vhost_socket *s;
	char *path;
	int ret;

	path = g_strdup_printf("%s/qemu-%s", sockets_path, state->brick.name);
	g_remove(path);

	printf("New vhost-user socket: %s, zero-copy %s\n", path,
	       (flags & RTE_VHOST_USER_DEQUEUE_ZERO_COPY) ?
	       "enable" : "disable");

	flags = flags & (RTE_VHOST_USER_CLIENT | RTE_VHOST_USER_NO_RECONNECT |
			 RTE_VHOST_USER_DEQUEUE_ZERO_COPY);

	ret = rte_vhost_driver_register(path, flags);

	if (ret) {
		*errp = pg_error_new_errno(-ret,
			"Error registering driver for socket %s",
			path);
		g_free(path);
		return;
	}

	pthread_mutex_lock(&mutex);

	s = g_new0(struct pg_vhost_socket, 1);
	s->path = path;

	state->socket = s;
	state->socket->state = state;

	LIST_INSERT_HEAD(&sockets, s, socket_list);

	pthread_mutex_unlock(&mutex);
}

#define VHOST_NOT_READY							\
	"vhost not ready, did you called vhost_start after packetgraph_start ?"

static int vhost_init(struct pg_brick *brick, struct pg_brick_config *config,
		      struct pg_error **errp)
{
	struct pg_vhost_state *state;
	struct pg_vhost_config *vhost_config;

	state = pg_brick_get_state(brick, struct pg_vhost_state);
	if (!vhost_start_ok) {
		*errp = pg_error_new(VHOST_NOT_READY);
		return -1;
	}

	vhost_config = (struct pg_vhost_config *) config->brick_config;
	state->output = vhost_config->output;
	state->vid = -1;
	rte_atomic64_set(&state->rx_bytes, 0);
	rte_atomic64_set(&state->tx_bytes, 0);

	vhost_create_socket(state, vhost_config->flags, errp);
	if (pg_error_is_set(errp))
		return -1;

	brick->burst = vhost_burst;
	brick->poll = vhost_poll;

	rte_atomic32_init(&state->allow_queuing);
	rte_atomic32_set(&state->allow_queuing, 1);

	return 0;
}

#undef VHOST_NOT_READY

static enum pg_side vhost_get_side(struct pg_brick *brick)
{
	struct pg_vhost_state *state =
	pg_brick_get_state(brick, struct pg_vhost_state);

	return pg_flip_side(state->output);
}

struct pg_brick *pg_vhost_new(const char *name, uint64_t flags,
			      struct pg_error **errp)
{
	struct pg_brick_config *config = vhost_config_new(name, flags);
	struct pg_brick *ret = pg_brick_new("vhost", config, errp);

	pg_brick_config_free(config);
	return ret;
}


static void vhost_destroy(struct pg_brick *brick, struct pg_error **errp)
{
	struct pg_vhost_state *state;

	state = pg_brick_get_state(brick, struct pg_vhost_state);
	rte_vhost_driver_unregister(state->socket->path);

	pthread_mutex_lock(&mutex);
	g_remove(state->socket->path);
	LIST_REMOVE(state->socket, socket_list);
	g_free(state->socket->path);
	g_free(state->socket);

	pthread_mutex_unlock(&mutex);
}

const char *pg_vhost_socket_path(struct pg_brick *brick,
				 struct pg_error **errp)
{
	struct pg_vhost_state *state;

	state = pg_brick_get_state(brick, struct pg_vhost_state);
	return state->socket->path;
}

static uint64_t rx_bytes(struct pg_brick *brick)
{
	return rte_atomic64_read(
		&pg_brick_get_state(brick, struct pg_vhost_state)->rx_bytes);
}

static uint64_t tx_bytes(struct pg_brick *brick)
{
	return rte_atomic64_read(
		&pg_brick_get_state(brick, struct pg_vhost_state)->tx_bytes);
}

static int new_vm(int dev)
{
	struct pg_vhost_socket *s;
	char buf[256];

	rte_vhost_get_ifname(dev, buf, 256);
	printf("VM connecting to socket: %s\n", buf);

	pthread_mutex_lock(&mutex);

	LIST_FOREACH(s, &sockets, socket_list) {
		if (!strcmp(s->path, buf)) {
			s->state->vid = dev;
			rte_atomic32_clear(&s->state->allow_queuing);
			break;
		}
	}

	pthread_mutex_unlock(&mutex);

	return 0;
}

static void destroy_vm(int dev)
{
	struct pg_vhost_socket *s;
	char buf[256];

	rte_vhost_get_ifname(dev, buf, 256);

	printf("VM disconnecting from socket: %s\n", buf);

	pthread_mutex_lock(&mutex);

	LIST_FOREACH(s, &sockets, socket_list) {
		if (!strcmp(s->path, buf)) {
			while (!rte_atomic32_test_and_set(&s->state->
							  allow_queuing))
				sched_yield();
			s->state->vid = -1;
			break;
		}
	}

	pthread_mutex_unlock(&mutex);

}

#undef CONCAT_VOLATILE
#undef VOLATILE

static const struct virtio_net_device_ops virtio_net_device_ops = {
	.new_device = new_vm,
	.destroy_device = destroy_vm,
};

/**
 * Check that the socket path exists and has the right perm and store it
 *
 * WARNING: As strlen is called on base_dir it should be a valid C string
 *
 * @param: the base directory where the vhost-user socket will be created
 * @param: an error pointer
 *
 */
static void check_and_store_base_dir(const char *base_dir,
				     struct pg_error **errp)
{
	char *resolved_path = g_malloc0(PATH_MAX);
	struct stat st;

	/* does the base directory exists ? */
	if ((lstat(base_dir, &st)) || !S_ISDIR(st.st_mode)) {
		*errp = pg_error_new("Invalid vhost-user socket directory %s",
				  base_dir);
		goto free_exit;
	}

	pthread_mutex_lock(&mutex);

	/* guard against implementation issues */
	/* Flawfinder: ignore we notified the user base_dir must be valid */
	if (strlen(base_dir) >= PATH_MAX) {
		*errp = pg_error_new("base_dir too long");
		goto unlock_mutex;
	}

	/* Flawfinder: ignore */
	sockets_path = realpath(base_dir, resolved_path);
	/* Neither base_dir nor resolved_path is smaller than PATH_MAX. */
	if (!sockets_path) {
		*errp = pg_error_new_errno(errno, "Cannot resolve path of %s",
					   base_dir);
		goto unlock_mutex;
	}

	sockets_path = g_strdup(sockets_path);

unlock_mutex:
	pthread_mutex_unlock(&mutex);
free_exit:
	g_free(resolved_path);
}

static void *vhost_session_thread_body(void *opaque)
{
	rte_vhost_driver_session_start();
	pthread_exit(NULL);
}

int pg_vhost_start(const char *base_dir, struct pg_error **errp)
{
	int ret;

	if (vhost_start_ok) {
		*errp = pg_error_new("vhost already started");
		return -1;
	}

	LIST_INIT(&sockets);

	check_and_store_base_dir(base_dir, errp);
	if (pg_error_is_set(errp))
		return -1;

	ret = rte_vhost_driver_callback_register(&virtio_net_device_ops);
	if (ret) {
		*errp = pg_error_new_errno(-ret,
			"Failed to register vhost-user callbacks");
		goto free_exit;
	}

	ret = pthread_create(&vhost_session_thread, NULL,
			     vhost_session_thread_body, NULL);

	if (ret) {
		*errp = pg_error_new_errno(-ret,
			"Failed to start vhost user thread");
		goto free_exit;
	}

	/* Store that vhost start was successful. */
	vhost_start_ok = 1;

	return 0;

free_exit:
	g_free(sockets_path);
	return -1;
}

int pg_vhost_enable(uint64_t feature_mask)
{
	return rte_vhost_feature_enable(feature_mask) ? -1 : 0;
}

int pg_vhost_disable(uint64_t feature_mask)
{
	return rte_vhost_feature_disable(feature_mask) ? -1 : 0;
}

static void vhost_link(struct pg_brick *brick, enum pg_side side, int edge)
{
	struct pg_vhost_state *state =
	pg_brick_get_state(brick, struct pg_vhost_state);

	state->output = pg_flip_side(side);
}

void pg_vhost_stop(void)
{
	void *ret;

	if (!vhost_start_ok)
		return;
	g_free(sockets_path);
	sockets_path = NULL;
	pthread_cancel(vhost_session_thread);
	pthread_join(vhost_session_thread, &ret);
	vhost_start_ok = 0;
}

static struct pg_brick_ops vhost_ops = {
	.name		= "vhost",
	.state_size	= sizeof(struct pg_vhost_state),

	.init		= vhost_init,
	.destroy	= vhost_destroy,

	.link_notify    = vhost_link,
	.get_side       = vhost_get_side,
	.unlink		= pg_brick_generic_unlink,
	.rx_bytes	= rx_bytes,
	.tx_bytes	= tx_bytes,
};

pg_brick_register(vhost, &vhost_ops);
