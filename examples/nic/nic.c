/* Copyright 2016 Outscale SAS
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

#include <packetgraph/packetgraph.h>

int main(int argc, char **argv)
{
	struct pg_brick *nic = NULL;
	struct pg_error *error = NULL;
	uint32_t rx, tx;
	int port_count;

	pg_start(argc, argv, &error);
	if (pg_error_is_set(&error)) {
		pg_error_print(error);
		pg_error_free(error);
		return 1;
	}

	port_count = pg_nic_port_count();
	if (port_count == 0) {
		printf("Error: you need at least one DPDK port\n");
		return 1;
	}

	for (int i = 0; i < port_count; i++) {
		nic = pg_nic_new_by_id("nic", i, &error);
		if (!nic) {
			printf("Error on nic creation (port %i)\n", i);
			pg_error_print(error);
			pg_error_free(error);
			pg_stop();
			return 1;
		}
		pg_nic_capabilities(nic, &rx, &tx);
		printf("====== Port %i capabilities ======\nRX:\n", i);
#define print_capa(R, capa) \
		printf(#capa ":\t\t%s\n", (R) & (capa) ? "yes" : "no");
		print_capa(rx, PG_NIC_RX_OFFLOAD_VLAN_STRIP);
		print_capa(rx, PG_NIC_RX_OFFLOAD_IPV4_CKSUM);
		print_capa(rx, PG_NIC_RX_OFFLOAD_UDP_CKSUM);
		print_capa(rx, PG_NIC_RX_OFFLOAD_TCP_CKSUM);
		print_capa(rx, PG_NIC_RX_OFFLOAD_TCP_LRO);
		print_capa(rx, PG_NIC_RX_OFFLOAD_QINQ_STRIP);
		print_capa(rx, PG_NIC_RX_OFFLOAD_OUTER_IPV4_CKSUM);
		printf("TX:\n");
		print_capa(tx, PG_NIC_TX_OFFLOAD_VLAN_INSERT);
		print_capa(tx, PG_NIC_TX_OFFLOAD_IPV4_CKSUM);
		print_capa(tx, PG_NIC_TX_OFFLOAD_UDP_CKSUM);
		print_capa(tx, PG_NIC_TX_OFFLOAD_TCP_CKSUM);
		print_capa(tx, PG_NIC_TX_OFFLOAD_SCTP_CKSUM);
		print_capa(tx, PG_NIC_TX_OFFLOAD_TCP_TSO);
		print_capa(tx, PG_NIC_TX_OFFLOAD_UDP_TSO);
		print_capa(tx, PG_NIC_TX_OFFLOAD_OUTER_IPV4_CKSUM);
		print_capa(tx, PG_NIC_TX_OFFLOAD_QINQ_INSERT);
#undef print_capa
		pg_brick_destroy(nic);
	}
	pg_stop();
	return 0;
}
