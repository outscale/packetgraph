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

#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <rte_config.h>
#include <rte_hash_crc.h>

#include "common.h"
#include "bricks/vhost-user-brick.h"
#include "packets/packets.h"
#include "utils/mempool.h"
#include "utils/bitmask.h"
#include "bricks/brick.h"
#include "tests.h"
#include "mac.h"

#define BUILDROOT "./3rdparty/buildroot/output/"
#define IMAGE BUILDROOT"/build/linux-3.18.4/arch/x86_64/boot/bzImage"
#define CPIO  BUILDROOT"/images/rootfs.cpio"

#define NB_PKTS	1

#define MEM "memory-backend-file,id=mem,size=1G,mem-path=/mnt/huge,share=on"

static int is_ready(char *str)
{
	if (!str)
		return 0;

	return !strncmp(str, "Ready", strlen("Ready"));
}

static int spawn_qemu(char *socket_path_0,
		      char *socket_path_1,
		      const char *mac_0,
		      const char *mac_1)
{
	GIOChannel *stdout_gio;
	GError *error = NULL;
	gsize terminator_pos;
	gchar *str_stdout;
	int  child_pid;
	gint stderr_fd;
	gint stdout_fd;
	gsize length;
	gchar **argv;

	argv     = g_new(gchar *, 33);
	argv[0]  = g_strdup("qemu-system-x86_64");
	argv[1]  = g_strdup("-m");
	argv[2]  = g_strdup("1G");
	argv[3]  = g_strdup("-enable-kvm");
	argv[4]  = g_strdup("-kernel");
	argv[5]  = g_strdup(IMAGE);
	argv[6]  = g_strdup("-initrd");
	argv[7]  = g_strdup(CPIO);
	argv[8]  = g_strdup("-append");
	argv[9]  = g_strdup("\"console=ttyS0 rdinit=/sbin/init\"");
	argv[10] = g_strdup("-serial");
	argv[11] = g_strdup("stdio");
	argv[12] = g_strdup("-monitor");
	argv[13] = g_strdup("/dev/null");
	argv[14] = g_strdup("-nographic");

	argv[15] = g_strdup("-chardev");
	argv[16] = g_strdup_printf("socket,id=char0,path=%s", socket_path_0);
	argv[17] = g_strdup("-netdev");
	argv[18] =
		g_strdup("type=vhost-user,id=mynet1,chardev=char0,vhostforce");
	argv[19] = g_strdup("-device");
	argv[20] = g_strdup_printf("virtio-net-pci,mac=%s,netdev=mynet1",
				   mac_0);
	argv[21] = g_strdup("-chardev");
	argv[22] = g_strdup_printf("socket,id=char1,path=%s", socket_path_1);
	argv[23] = g_strdup("-netdev");
	argv[24] =
		g_strdup("type=vhost-user,id=mynet2,chardev=char1,vhostforce");
	argv[25] = g_strdup("-device");
	argv[26] = g_strdup_printf("virtio-net-pci,mac=%s,netdev=mynet2",
				   mac_1);
	argv[27] = g_strdup("-object");
	argv[28] = g_strdup(MEM);
	argv[29] = g_strdup("-numa");
	argv[30] = g_strdup("node,memdev=mem");
	argv[31] = g_strdup("-mem-prealloc");

	argv[32] = NULL;

	if (!g_spawn_async_with_pipes(NULL,
				      argv,
				      NULL,
				      (GSpawnFlags) G_SPAWN_SEARCH_PATH |
						    G_SPAWN_DO_NOT_REAP_CHILD,
				      (GSpawnChildSetupFunc) NULL,
				      NULL,
				      &child_pid,
				      NULL,
				      &stdout_fd,
				      &stderr_fd,
				      &error))
		g_assert(0);

	g_strfreev(argv);

	stdout_gio = g_io_channel_unix_new(stdout_fd);

	g_io_channel_read_line(stdout_gio,
				&str_stdout,
				&length,
				&terminator_pos,
				&error);
	while (!is_ready(str_stdout)) {
		g_free(str_stdout);
		g_io_channel_read_line(stdout_gio,
					&str_stdout,
					&length,
					&terminator_pos,
					&error);
	}
	g_free(str_stdout);

	g_io_channel_shutdown(stdout_gio, TRUE, &error);
	g_io_channel_unref(stdout_gio);

	return child_pid;
}

/* this test harness a Linux guest to check that packet are send and received
 * by the vhost brick. An ethernet bridge inside the guest will forward packets
 * between the two vhost-user virtio interfaces.
 */
static void test_vhost_flow(void)
{
	struct brick_config *collect_config = brick_config_new("collect", 1, 1);
	struct brick_config *vhost_config_0 = vhost_config_new("vhost-0",
							1,
							1,
							EAST_SIDE);
	struct brick_config *vhost_config_1 = vhost_config_new("vhost-1",
							1,
							1,
							EAST_SIDE);
	const char mac_addr_0[18] = "52:54:00:12:34:11";
	const char mac_addr_1[18] = "52:54:00:12:34:12";
	struct rte_mempool *mbuf_pool = get_mempool();
	struct brick *vhost_0, *vhost_1, *collect;
	struct rte_mbuf *pkts[MAX_PKTS_BURST];
	char *socket_path_0, *socket_path_1;
	struct switch_error *error = NULL;
	struct rte_mbuf **result_pkts;
	int ret, qemu_pid, i;
	uint64_t pkts_mask;

	/* start vhost */
	ret = vhost_start("/tmp", &error);
	g_assert(ret);
	g_assert(!error);


	/* instanciate brick */
	vhost_0 = brick_new("vhost-user", vhost_config_0, &error);
	g_assert(!error);
	g_assert(vhost_0);

	vhost_1 = brick_new("vhost-user", vhost_config_1, &error);
	g_assert(!error);
	g_assert(vhost_1);

	collect = brick_new("collect", collect_config, &error);
	g_assert(!error);
	g_assert(collect);

	/* build the graph */
	brick_east_link(collect, vhost_1, &error);
	g_assert(!error);

	/* spawn first QEMU */
	socket_path_0 = brick_handle_dup(vhost_0, &error);
	g_assert(!error);
	g_assert(socket_path_0);
	socket_path_1 = brick_handle_dup(vhost_1, &error);
	g_assert(!error);
	g_assert(socket_path_1);
	qemu_pid = spawn_qemu(socket_path_0, socket_path_1,
			      mac_addr_0, mac_addr_1);
	g_assert(qemu_pid);
	g_free(socket_path_0);
	g_free(socket_path_1);

	/* prepare packet to send */
	for (i = 0; i < NB_PKTS; i++) {
		pkts[i] = rte_pktmbuf_alloc(mbuf_pool);
		g_assert(pkts[i]);
		rte_pktmbuf_append(pkts[i], ETHER_MIN_LEN);
		/* set random dst/src mac address so the linux guest bridge
		 * will not filter them
		 */
		set_mac_addrs(pkts[i],
			      "52:54:00:12:34:15", "52:54:00:12:34:16");
		/* set size */
		set_ether_type(pkts[i], ETHER_MIN_LEN - ETHER_HDR_LEN - 4);
	}

	/* send packet to the guest via one interface */
	brick_burst_to_east(vhost_0, 0, pkts, NB_PKTS, mask_firsts(NB_PKTS),
			    &error);
	g_assert(!error);

	/* let the packet propagate and flow */
	for (i = 0; i < 10; i++) {
		uint16_t count = 0;

		usleep(100000);
		brick_poll(vhost_1, &count, &error);
		g_assert(!error);
		if (count)
			break;
	}

	/* kill QEMU */
	g_spawn_close_pid(qemu_pid);

	result_pkts = brick_east_burst_get(collect, &pkts_mask, &error);
	g_assert(!error);
	g_assert(result_pkts);

	/* free result packets */
	packets_free(result_pkts, pkts_mask);

	/* free sent packet */
	for (i = 0; i < NB_PKTS; i++)
		rte_pktmbuf_free(pkts[i]);

	/* break the graph */
	brick_unlink(collect, &error);
	g_assert(!error);

	/* clean up */
	brick_decref(vhost_0, &error);
	g_assert(!error);
	brick_decref(vhost_1, &error);
	g_assert(!error);
	brick_decref(collect, &error);
	g_assert(!error);

	brick_config_free(vhost_config_0);
	brick_config_free(vhost_config_1);
	brick_config_free(collect_config);

	/* stop vhost */
	vhost_stop();
}

void test_vhost(void)
{
	g_test_add_func("/vhost/flow", test_vhost_flow);
}
