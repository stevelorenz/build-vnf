/*
 * device.c
 */

#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_log.h>

#include "device.h"

int dpdk_init_device(struct dpdk_device_config *cfg)
{
	int ret = 0;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_rxconf rxq_conf;
	struct rte_eth_txconf txq_conf;
	struct rte_ether_addr port_eth_addr;

	/* TODO: Add support for multiple RX TX queues using flow director
         * <01-03-19, Zuo> */
	if (cfg->rx_queues > 1 || cfg->tx_queues > 2) {
		rte_exit(EXIT_FAILURE, "Support only one TX and RX queue.\n");
	}

	rte_eth_dev_info_get(cfg->port_id, &dev_info);
	RTE_LOG(INFO, PORT,
		"[PORT INFO] Port ID: %d, Driver name: %s, Number of RX queues: "
		"%d, TX queues: %d\n",
		cfg->port_id, dev_info.driver_name, dev_info.nb_rx_queues,
		dev_info.nb_tx_queues);

	// Use a generic port_conf by default
	struct rte_eth_conf port_conf = {
		.rxmode =
			{
				.max_rx_pkt_len = 9000,
			},
		.txmode =
			{
				.mq_mode = ETH_MQ_TX_NONE,
			},

	};
	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE) {
		RTE_LOG(INFO, PORT,
			"[PORT INFO] Port ID: %d support fast mbuf free.\n",
			cfg->port_id);
		port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
	}

	ret = rte_eth_dev_configure(cfg->port_id, cfg->rx_queues,
				    cfg->tx_queues, &port_conf);

	if (ret < 0) {
		rte_exit(EXIT_FAILURE,
			 "Cannot configure device: err=%d, port=%d\n", ret,
			 cfg->port_id);
	}

	// Check that numbers of Rx and Tx descriptors satisfy descriptors
	// limits from the ethernet device information, otherwise adjust them to
	// boundaries.
	ret = rte_eth_dev_adjust_nb_rx_tx_desc(cfg->port_id, &(cfg->rx_descs),
					       &(cfg->tx_descs));

	if (ret < 0) {
		rte_exit(
			EXIT_FAILURE,
			"Cannot adjust number of descriptors: err=%d, port=%u\n",
			ret, cfg->port_id);
	}
	rte_eth_macaddr_get(cfg->port_id, &port_eth_addr);

	// Allocate and setup RX and TX queues for a device
	rte_eth_dev_info_get(cfg->port_id, &dev_info);

	rxq_conf = dev_info.default_rxconf;
	rxq_conf.offloads = port_conf.rxmode.offloads;
	ret = rte_eth_rx_queue_setup(cfg->port_id, 0, cfg->rx_descs,
				     rte_eth_dev_socket_id(cfg->port_id),
				     &rxq_conf, *(cfg->pool));
	if (unlikely(ret != 0)) {
		rte_exit(EXIT_FAILURE,
			 "Can not setup rx queue with error code:%d\n", ret);
	}

	txq_conf = dev_info.default_txconf;
	txq_conf.offloads = port_conf.txmode.offloads;
	ret = rte_eth_tx_queue_setup(cfg->port_id, 0, cfg->tx_descs,
				     rte_eth_dev_socket_id(cfg->port_id),
				     &txq_conf);
	if (unlikely(ret != 0)) {
		rte_exit(EXIT_FAILURE,
			 "Can not setup tx queue with error code:%d\n", ret);
	}

	// Start device
	ret = rte_eth_dev_start(cfg->port_id);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Cannot start device: err=%d, port=%u\n",
			 ret, cfg->port_id);
	}
	rte_eth_promiscuous_enable(cfg->port_id);
	RTE_LOG(INFO, PORT,
		"Port:%d started, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
		cfg->port_id, port_eth_addr.addr_bytes[0],
		port_eth_addr.addr_bytes[1], port_eth_addr.addr_bytes[2],
		port_eth_addr.addr_bytes[3], port_eth_addr.addr_bytes[4],
		port_eth_addr.addr_bytes[5]);

	return 0;
}

void dpdk_cleanup_devices(void)
{
	uint32_t port_id;
	RTE_ETH_FOREACH_DEV(port_id)
	{
		rte_eth_dev_stop(port_id);
		rte_eth_dev_close(port_id);
	}
}
