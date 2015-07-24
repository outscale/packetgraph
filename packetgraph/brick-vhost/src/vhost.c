/* Copyright 2015 Nodalink EURL
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <urcu.h>

#include <rte_config.h>
#include <linux/virtio_net.h>
#include <linux/virtio_ring.h>
#include <librte_vhost/rte_virtio_net.h>

#include <packetgraph/common.h>
#include <packetgraph/brick.h>
#include <packetgraph/vhost.h>
#include <packetgraph/packets.h>
#include <packetgraph/utils/bitmask.h>
#include <packetgraph/utils/mempool.h>

#define MAX_BURST 32

struct vhost_config {
	enum side output;
};

struct socket {
	char *path;			/* path of the socket */
	struct vhost_state *state;	/* NULL if the socket is free */

	LIST_ENTRY(socket) socket_list; /* sockets list */
};

struct vhost_state {
	struct brick brick;
	enum side output;
	struct socket *socket;
	struct virtio_net *virtio_net;	/* fast path sync (protected by RCU) */
	struct rte_mbuf *in[MAX_PKTS_BURST];
	struct rte_mbuf *out[MAX_PKTS_BURST];
};

static LIST_HEAD(socket_list, socket) sockets; /* head of the socket list */
static char *sockets_path;
static int vhost_start_ok;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t vhost_session_thread;

static struct brick_config *vhost_config_new(const char *name,
					     uint32_t west_max,
					     uint32_t east_max,
					     enum side output)
{
	struct brick_config *config = g_new0(struct brick_config, 1);
	struct vhost_config *vhost_config = g_new0(struct vhost_config, 1);

	vhost_config->output = output;
	config->brick_config = (void *) vhost_config;
	return brick_config_init(config, name, west_max, east_max);
}

static int vhost_burst(struct brick *brick, enum side from, uint16_t edge_index,
		       struct rte_mbuf **pkts, uint16_t nb, uint64_t pkts_mask,
		       struct switch_error **errp)
{
	struct vhost_state *state = brick_get_state(brick, struct vhost_state);
	struct virtio_net *virtio_net;
	uint16_t pkts_count;

	if (state->output == from) {
		*errp = error_new("Burst packets going on the wrong direction");
		return 0;
	}

	rcu_read_lock();
	virtio_net = rcu_dereference(state->virtio_net);

	if (unlikely(!virtio_net)) {
		rcu_read_unlock();
		return 1;
	}

	packets_incref(pkts, pkts_mask);
	pkts_count = packets_pack(state->out, pkts, pkts_mask);
	rte_vhost_enqueue_burst(virtio_net, VIRTIO_RXQ, state->out,
					 pkts_count);

	rcu_read_unlock();
	packets_free(state->out, mask_firsts(pkts_count));

	return 1;
}

static int vhost_poll(struct brick *brick, uint16_t *pkts_cnt,
		      struct switch_error **errp)
{
	struct vhost_state *state = brick_get_state(brick, struct vhost_state);
	struct brick_side *s = &brick->sides[flip_side(state->output)];
	struct rte_mempool *mp = get_mempool();
	struct virtio_net *virtio_net;
	uint64_t pkts_mask;
	uint16_t count;
	int ret;

	rcu_read_lock();
	virtio_net = rcu_dereference(state->virtio_net);

	if (unlikely(!virtio_net)) {
		rcu_read_unlock();
		return 1;
	}

	count = rte_vhost_dequeue_burst(virtio_net, VIRTIO_TXQ, mp, state->in,
					MAX_BURST);
	*pkts_cnt = count;

	rcu_read_unlock();

	if (!count)
		return 1;

	pkts_mask = mask_firsts(count);
	ret = brick_side_forward(s, state->output,
				 state->in, count, pkts_mask, errp);
	packets_free(state->in, pkts_mask);

	return ret;
}

static void vhost_create_socket(struct vhost_state *state,
				struct switch_error **errp)
{
	struct socket *s;
	char *path;
	int ret;

	path = g_strdup_printf("%s/qemu-%s", sockets_path, state->brick.name);

	printf("New vhost-user socket: %s\n", path);

	ret = rte_vhost_driver_register(path);

	if (ret) {
		*errp = error_new_errno(-ret,
			"Error registering driver for socket %s",
			path);
		g_free(path);
		return;
	}

	pthread_mutex_lock(&mutex);

	s = g_new0(struct socket, 1);
	s->path = path;

	state->socket = s;
	state->socket->state = state;

	LIST_INSERT_HEAD(&sockets, s, socket_list);

	pthread_mutex_unlock(&mutex);
}

static int vhost_init(struct brick *brick, struct brick_config *config,
		      struct switch_error **errp)
{
	struct vhost_state *state = brick_get_state(brick, struct vhost_state);
	struct vhost_config *vhost_config;

	if (!vhost_start_ok) {
		*errp = error_new(
"vhost not ready, did you called vhost_start after packetgraph_start ?");
		return 0;
	}

	if (!config) {
		*errp = error_new("config is NULL");
		return 0;
	}

	if (!config->brick_config) {
		*errp = error_new("config->brick_config is NULL");
		return 0;
	}

	vhost_config = (struct vhost_config *) config->brick_config;

	state->output = vhost_config->output;

	if (error_is_set(errp))
		return 0;

	vhost_create_socket(state, errp);

	if (error_is_set(errp))
		return 0;

	brick->burst = vhost_burst;
	brick->poll = vhost_poll;

	return 1;
}

struct brick *vhost_new(const char *name, uint32_t west_max,
			uint32_t east_max, enum side output,
			struct switch_error **errp)
{
	struct brick_config *config = vhost_config_new(name, west_max,
						       east_max, output);
	struct brick *ret = brick_new("vhost", config, errp);

	brick_config_free(config);
	return ret;
}


static void vhost_destroy(struct brick *brick, struct switch_error **errp)
{
	struct vhost_state *state = brick_get_state(brick, struct vhost_state);

	pthread_mutex_lock(&mutex);

	/* TODO: uncomment when implemented
	 * rte_vhost_driver_unregister(state->socket->path);
	 */
	LIST_REMOVE(state->socket, socket_list);
	g_free(state->socket->path);
	g_free(state->socket);

	pthread_mutex_unlock(&mutex);
}

static char *vhost_handle_dup(struct brick *brick, struct switch_error **errp)
{
	struct vhost_state *state = brick_get_state(brick, struct vhost_state);

	return g_strdup(state->socket->path);
}

static struct brick_ops vhost_ops = {
	.name		= "vhost",
	.state_size	= sizeof(struct vhost_state),

	.init		= vhost_init,
	.destroy	= vhost_destroy,

	.unlink		= brick_generic_unlink,

	.handle_dup	= vhost_handle_dup,
};

brick_register(vhost, &vhost_ops);

static int new_vm(struct virtio_net *dev)
{
	struct socket *s;

	printf("VM connecting to socket: %s\n", dev->ifname);

	pthread_mutex_lock(&mutex);

	LIST_FOREACH(s, &sockets, socket_list) {
		if (!strcmp(s->path, dev->ifname)) {
			rcu_assign_pointer(s->state->virtio_net, dev);
			synchronize_rcu();
			break;
		}
	}

	pthread_mutex_unlock(&mutex);

	dev->flags |= VIRTIO_DEV_RUNNING;
	return 0;
}

/* silence checkpatch.pl warning for volatile */
#define CONCAT_VOLATILE(X, Y) X##Y
#define VOLATILE CONCAT_VOLATILE(vola, tile)

static void destroy_vm(VOLATILE struct virtio_net *dev)
{
	const char *path = (char *) dev->ifname;
	struct socket *s;

	printf("VM disconnecting from socket: %s\n", path);

	pthread_mutex_lock(&mutex);

	LIST_FOREACH(s, &sockets, socket_list) {
		if (!strcmp(s->path, path)) {
			rcu_assign_pointer(s->state->virtio_net, NULL);
			synchronize_rcu();
			break;
		}
	}

	pthread_mutex_unlock(&mutex);

	dev->flags &= ~VIRTIO_DEV_RUNNING;
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
				     struct switch_error **errp)
{
	char *resolved_path = g_malloc0(PATH_MAX);
	struct stat st;

	/* does the base directory exists ? */
	if ((lstat(base_dir, &st)) || !S_ISDIR(st.st_mode)) {
		*errp = error_new("Invalid vhost-user socket directory %s",
				  base_dir);
		goto free_exit;
	}

	pthread_mutex_lock(&mutex);

	/* guard against implementation issues */
	/* Flawfinder: ignore we notified the user base_dir must be valid */
	if (strlen(base_dir) >= PATH_MAX) {
		*errp = error_new("base_dir too long");
		goto free_exit;
	}

	/* Flawfinder: ignore */
	sockets_path = realpath(base_dir, resolved_path);
	/* Neither base_dir nor resolved_path is smaller than PATH_MAX. */
	if (!sockets_path) {
		pthread_mutex_unlock(&mutex);
		*errp = error_new_errno(errno, "Cannot resolve path of %s",
					base_dir);
		goto free_exit;
	}

	sockets_path = g_strdup(sockets_path);

	pthread_mutex_unlock(&mutex);

free_exit:
	g_free(resolved_path);
}

static void *vhost_session_thread_body(void *opaque)
{
	rte_vhost_driver_session_start();
	pthread_exit(NULL);
}

int vhost_start(const char *base_dir, struct switch_error **errp)
{
	int ret;

	rcu_register_thread();

	rte_vhost_feature_enable(1ULL << VIRTIO_NET_F_CTRL_RX);

	LIST_INIT(&sockets);

	check_and_store_base_dir(base_dir, errp);
	if (error_is_set(errp))
		return 0;

	ret = rte_vhost_driver_callback_register(&virtio_net_device_ops);
	if (ret) {
		*errp = error_new_errno(-ret,
			"Failed to register vhost-user callbacks");
		goto free_exit;
	}

	ret = pthread_create(&vhost_session_thread, NULL,
			     vhost_session_thread_body, NULL);

	if (ret) {
		*errp = error_new_errno(-ret,
			"Failed to start vhost user thread");
		goto free_exit;
	}

	/* Store that vhost start was successful. */
	vhost_start_ok = 1;

	return 1;

free_exit:
	g_free(sockets_path);
	return 0;
}

void vhost_stop(void)
{
	void *ret;

	pthread_cancel(vhost_session_thread);
	pthread_join(vhost_session_thread, &ret);
	rcu_unregister_thread();
}
