/*
 * About:
 */

#include <inttypes.h>
#include <stdint.h>

#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include <fastio_user/utils.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NUM_MBUFS 5000
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.max_rx_pkt_len = 9000,
	},
};

static inline int port_init(uint16_t port, struct rte_mempool* mbuf_pool)
{
        struct rte_eth_conf port_conf = port_conf_default;
        const uint16_t rx_rings = 2, tx_rings = 4;
        uint16_t nb_rxd = RX_RING_SIZE;
        uint16_t nb_txd = TX_RING_SIZE;
        int retval;

        if (port >= 2)
                return -1;

        retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
        if (retval != 0)
                return retval;

        retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
        if (retval != 0)
                return retval;

        for (uint16_t q = 0; q < rx_rings; q++) {
                retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
                    rte_eth_dev_socket_id(port), NULL, mbuf_pool);
                if (retval < 0)
                        return retval;
        }

        for (uint16_t q = 0; q < tx_rings; q++) {
                retval = rte_eth_tx_queue_setup(
                    port, q, nb_txd, rte_eth_dev_socket_id(port), NULL);
                if (retval < 0)
                        return retval;
        }

        retval = rte_eth_dev_start(port);
        if (retval < 0)
                return retval;

        rte_eth_promiscuous_enable(port);
        return 0;
}

static __attribute__((noreturn)) void lcore_main(void)
{
        const uint16_t nb_ports = 1;
        uint16_t port;

        for (port = 0; port < nb_ports; port++)
                if (rte_eth_dev_socket_id(port) > 0
                    && rte_eth_dev_socket_id(port) != (int)rte_socket_id())
                        printf("WARNING, port %u is on remote NUMA node to "
                               "polling thread.\n\tPerformance will "
                               "not be optimal.\n",
                            port);

        printf(
            "\nCore %u forwarding packets. [Ctrl+C to quit]\n", rte_lcore_id());

        for (;;) {
                for (port = 0; port < nb_ports; port++) {
                        for (uint16_t qid = 0; qid < 2; qid++) {

                                struct rte_mbuf* bufs[BURST_SIZE];
                                const uint16_t nb_rx = rte_eth_rx_burst(
                                    port, qid, bufs, BURST_SIZE);
                                if (unlikely(nb_rx == 0))
                                        continue;

                                for (uint16_t i = 0; i < nb_rx; i++) {
                                        printf(
                                            "port%u:%u recv packet len=%u \n",
                                            port, qid,
                                            rte_pktmbuf_pkt_len(bufs[i]));
                                        /* rte_pktmbuf_dump(stdout, bufs[i], 0);
                                         */
                                }

                                const uint16_t nb_tx = rte_eth_tx_burst(
                                    port ^ 1, qid, bufs, nb_rx);
                                if (unlikely(nb_tx < nb_rx)) {
                                        uint16_t buf;
                                        for (buf = nb_tx; buf < nb_rx; buf++)
                                                rte_pktmbuf_free(bufs[buf]);
                                }
                        }
                }
        }
}

/**
 * @brief main
 *
 */
int main(int argc, char* argv[])
{
        struct rte_mempool* mbuf_pool;
        uint16_t portid;

        int ret = rte_eal_init(argc, argv);
        if (ret < 0)
                rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

        const unsigned nb_ports = 1;
        printf("%u ports were found\n", nb_ports);
        if (nb_ports != 2)
                rte_exit(EXIT_FAILURE, "Error: number of ports is not 2\n");

        mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
            MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

        if (mbuf_pool == NULL)
                rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

        for (portid = 0; portid < nb_ports; portid++)
                if (port_init(portid, mbuf_pool) != 0)
                        rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n",
                            portid);

        if (rte_lcore_count() > 1)
                printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

        rte_exit(EXIT_FAILURE, "Finish the port init process.\n");

        lcore_main();
        return 0;
}
