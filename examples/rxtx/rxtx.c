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
#include <glib/gprintf.h>
#include <packetgraph/packetgraph.h>

/* Let's build a simple server:
 *
 * [nic]--[rxtx]
 *
 * The server would just print time arrival of all packets.
 * App will send packets containing the number of received packets per seconds
 */

uint64_t start_time;

struct mydata {
	uint64_t pkt_count;
};

static void rx_callback(struct pg_brick *brick,
			pg_packet_t **rx_burst,
			uint16_t rx_burst_len,
			void *private_data)
{
	struct mydata *pd = (struct mydata *)private_data;
	uint64_t current = g_get_real_time();

	for (int i = 0; i < rx_burst_len; i++) {
		/* note: you can access packet data with
		 * pg_packet_data(rx_burst[i]) */
		g_printf("[%lf] received a packet of len %i\n",
			 (current - start_time) / 1000000.,
			 pg_packet_len(rx_burst[i]));
	}
	pd->pkt_count += rx_burst_len;
}

static void tx_callback(struct pg_brick *brick,
			pg_packet_t **tx_burst,
			uint16_t *tx_burst_len,
			void *private_data)
{
	static uint64_t pkt_count;
	struct mydata *pd = (struct mydata *)private_data;

	if (pkt_count == pd->pkt_count) {
		*tx_burst_len = 0;
		return;
	}
	pkt_count = pd->pkt_count;
	uint64_t current_time = g_get_real_time();

	/* set number of packet we would like to write.
	 * maximal number of packets is PG_RXTX_MAX_TX_BURST_LEN
	 */;
	*tx_burst_len = 1;
	/* write the first packets data */
	*((uint64_t *)(pg_packet_data(tx_burst[0]))) =
		pd->pkt_count / (current_time - start_time);
	pg_packet_set_len(tx_burst[0], sizeof(uint64_t));
	printf("%lf packets/s\n",
	       pd->pkt_count / ((current_time - start_time) / 1000000.));
}

int main(int argc, char **argv)
{
	struct pg_error *error = NULL;
	struct pg_brick *nic, *app;
	struct pg_graph *graph;
	struct mydata pd;

	pd.pkt_count = 0;
	pg_start(argc, argv, &error);
	start_time = g_get_real_time();

	if (pg_nic_port_count() < 1) {
		g_printf("No NIC port has been found, try adding "
			"--vdev=eth_pcap0,iface=eth0 arguments\n");
		return 1;
	}

	nic = pg_nic_new_by_id("port 0", 0, &error);
	app = pg_rxtx_new("myapp", &rx_callback, &tx_callback, (void *) &pd);
	pg_brick_link(nic, app, &error);

	graph = pg_graph_new("example", nic, &error);

	while (42)
		if (pg_graph_poll(graph, &error) < 0)
			break;
	pg_error_print(error);
	pg_error_free(error);
	pg_graph_destroy(graph);
	pg_stop();
	return 1;
}

#undef CHECK_ERROR
