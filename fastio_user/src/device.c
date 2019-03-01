/*
 * device.c
 */

#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_log.h>

#include "device.h"

int dpdk_init_device(struct dpdk_device_config* cfg)
{
        int ret = 0;
        struct rte_eth_dev_info dev_info;
        struct rte_eth_rxconf rxq_conf;
        struct rte_eth_txconf txq_conf;
        struct ether_addr port_eth_addr;

        /* TODO: Add support for multiple RX TX queues using flow director
         * <01-03-19, Zuo> */
        if (cfg->rx_queues > 1 || cfg->tx_queues > 2) {
                rte_exit(EXIT_FAILURE, "Support only one TX and RX queue.\n");
        }

        rte_eth_dev_info_get(cfg->portid, &dev_info);
        RTE_LOG(INFO, PORT,
            "[PORT INFO] Port ID: %d, Driver name: %s, Number of RX queues: "
            "%d, TX queues: %d\n",
            cfg->portid, dev_info.driver_name, dev_info.nb_rx_queues,
            dev_info.nb_tx_queues);

        // Use a generic port_conf by default
        struct rte_eth_conf port_conf = {
                .rxmode = {
                        .max_rx_pkt_len = 9000,
                },
                .txmode = {
                        .mq_mode = ETH_MQ_TX_NONE,
                },

        };
        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE) {
                RTE_LOG(INFO, PORT,
                    "[PORT INFO] Port ID: %d support fast mbuf free.\n",
                    cfg->portid);
                port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
        }

        ret = rte_eth_dev_configure(
            cfg->portid, cfg->rx_queues, cfg->tx_queues, &port_conf);

        if (ret < 0) {
                rte_exit(EXIT_FAILURE,
                    "Cannot configure device: err=%d, port=%d\n", ret,
                    cfg->portid);
        }

        // Check that numbers of Rx and Tx descriptors satisfy descriptors
        // limits from the ethernet device information, otherwise adjust them to
        // boundaries.
        ret = rte_eth_dev_adjust_nb_rx_tx_desc(
            cfg->portid, &(cfg->rx_descs), &(cfg->tx_descs));

        if (ret < 0) {
                rte_exit(EXIT_FAILURE,
                    "Cannot adjust number of descriptors: err=%d, port=%u\n",
                    ret, cfg->portid);
        }
        rte_eth_macaddr_get(cfg->portid, &port_eth_addr);

        // Allocate and setup RX and TX queues for a device
        rte_eth_dev_info_get(cfg->portid, &dev_info);

        rxq_conf = dev_info.default_rxconf;
        rxq_conf.offloads = port_conf.rxmode.offloads;
        ret = rte_eth_rx_queue_setup(cfg->portid, 0, cfg->rx_descs,
            rte_eth_dev_socket_id(cfg->portid), &rxq_conf, *(cfg->pool));

        txq_conf = dev_info.default_txconf;
        txq_conf.offloads = port_conf.txmode.offloads;
        ret = rte_eth_tx_queue_setup(cfg->portid, 0, cfg->tx_descs,
            rte_eth_dev_socket_id(cfg->portid), &txq_conf);

        // Start device
        ret = rte_eth_dev_start(cfg->portid);
        if (ret < 0) {
                rte_exit(EXIT_FAILURE, "Cannot start device: err=%d, port=%u\n",
                    ret, cfg->portid);
        }
        rte_eth_promiscuous_enable(cfg->portid);
        RTE_LOG(INFO, PORT,
            "Port:%d started, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
            cfg->portid, port_eth_addr.addr_bytes[0],
            port_eth_addr.addr_bytes[1], port_eth_addr.addr_bytes[2],
            port_eth_addr.addr_bytes[3], port_eth_addr.addr_bytes[4],
            port_eth_addr.addr_bytes[5]);

        return 0;
}
