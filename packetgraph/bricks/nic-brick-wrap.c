#include <rte_config.h>
#include <rte_ethdev.h>
#include "bricks/stubs.h"

uint16_t rte_eth_tx_burst_wrap(uint8_t port_id,
			       uint16_t queue_id,
			       struct rte_mbuf **tx_pkts,
			       uint16_t nb_pkts)
{
	return rte_eth_tx_burst(port_id, queue_id, tx_pkts, nb_pkts);
}
