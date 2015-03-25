#ifndef _STUB_H_
#define _STUB_H_

uint16_t rte_eth_tx_burst_wrap(uint8_t  port_id,
			       uint16_t  queue_id,
			       struct rte_mbuf **tx_pkts,
			       uint16_t  nb_pkts);

#endif
