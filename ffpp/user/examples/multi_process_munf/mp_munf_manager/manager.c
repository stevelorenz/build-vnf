/*
 * manager.c
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rte_eal.h>

#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ring.h>

#include <ffpp/collections.h>
#include <ffpp/config.h>
#include <ffpp/general_helpers_user.h>
#include <ffpp/munf.h>
#include <ffpp/packet_processors.h>

#define BURST_SIZE 64

static bool TEST_MODE = false;

static volatile bool force_quit = false;

static struct rte_ether_addr tx_port_addr;

static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		force_quit = true;
	}
}

static void run_enqueue_loop(struct rte_mbuf **buf, struct rte_ring *rx_ring)
{
	uint16_t nb_eq = 0;
	for (;;) {
		nb_eq = rte_ring_enqueue_bulk(rx_ring, (void **)buf, BURST_SIZE,
					      NULL);
		if (force_quit == true || nb_eq == BURST_SIZE) {
			break;
		}
		rte_delay_us_block(1e3);
	}
}

static void run_dequeue_loop(struct rte_mbuf **buf, struct rte_ring *rx_ring)
{
	uint16_t nb_dq = 0;
	for (;;) {
		nb_dq = rte_ring_dequeue_bulk(rx_ring, (void **)buf, BURST_SIZE,
					      NULL);
		if (force_quit == true || nb_dq == BURST_SIZE) {
			break;
		}
		rte_delay_us_block(1e3);
	}
}

static void run_mainloop(const struct ffpp_munf_manager *ctx,
			 const struct ffpp_munf_data *munf_data)
{
	struct rte_mbuf *rx_buf[BURST_SIZE];
	struct rte_mbuf *tx_buf[BURST_SIZE];
	uint16_t nb_rx, nb_tx;

	struct ffpp_mvec vec;
	ffpp_mvec_init(&vec, BURST_SIZE);
	uint16_t i;

	printf("The name of the RX ring: %s\n", munf_data->rx_ring_name);
	printf("The name of the TX ring: %s\n", munf_data->tx_ring_name);

	struct rte_ring *rx_ring = rte_ring_lookup(munf_data->rx_ring_name);
	struct rte_ring *tx_ring = rte_ring_lookup(munf_data->tx_ring_name);
	if (rx_ring == NULL || tx_ring == NULL) {
		rte_exit(EXIT_FAILURE, "Can not find the RX or TX ring!\n");
	}

	while (!force_quit) {
		nb_rx = rte_eth_rx_burst(ctx->rx_port_id, 0, rx_buf,
					 BURST_SIZE);
		if (nb_rx == 0) {
			continue;
		}
		RTE_LOG(DEBUG, FFPP, "Receive %u packets!\n", nb_rx);
		run_enqueue_loop(rx_buf, rx_ring);
		// TODO: Implement the basic scaling here based on the enqueue
		// statistics.

		if (unlikely(TEST_MODE == true)) {
			struct rte_mbuf *tmp_buf[BURST_SIZE];
			rte_ring_dequeue_bulk(rx_ring, (void **)tmp_buf,
					      BURST_SIZE, NULL);
			rte_ring_enqueue_bulk(tx_ring, (void **)tmp_buf,
					      BURST_SIZE, NULL);
		}

		run_dequeue_loop(tx_buf, tx_ring);

		ffpp_mvec_set_mbufs(&vec, rx_buf, nb_rx);
		ffpp_pp_update_dl_dst(&vec, &tx_port_addr);
		// No buffering is used like l2fwd.
		rte_eth_tx_burst(ctx->tx_port_id, 0, rx_buf, nb_rx);
		RTE_LOG(DEBUG, FFPP, "Finish one RX/TX round!\n");
	}
	ffpp_mvec_free(&vec);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	argc -= ret;
	argv += ret;
	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	struct ffpp_munf_manager munf_manager;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_manager(&munf_manager, "test_manager", pool);
	ret = rte_eth_macaddr_get(munf_manager.tx_port_id, &tx_port_addr);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Cannot get the MAC address.\n");
	}

	// Currently hard-coded.
	struct ffpp_munf_data munf_data;
	ffpp_munf_register("munf_1", &munf_data);

	run_mainloop(&munf_manager, &munf_data);

	ffpp_munf_unregister("munf_1");
	ffpp_munf_cleanup_manager(&munf_manager);
	rte_eal_cleanup();
	return 0;
}
