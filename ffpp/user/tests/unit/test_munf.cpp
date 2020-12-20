#include <rte_eal.h>
#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ring.h>

#include "ffpp/munf.h"

int main(int argc, char *argv[])
{
	if (rte_eal_init(argc, argv) < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	struct ffpp_munf_manager munf_manager;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_manager(&munf_manager, "test_manager", pool);

	struct ffpp_munf_data data1;
	ffpp_munf_register("test_munf_1", &data1);

	// Test IO with rings.
	uint16_t rx_count;
	struct rte_mbuf *rx_buf[17];
	struct rte_mbuf *tx_buf[17];
	rx_count = rte_eth_rx_burst(munf_manager.rx_port_id, 0, rx_buf, 17);
	if (rx_count != 17) {
		fprintf(stderr,
			"Failed to receive packets from the RX port.\n");
		return -1;
	}
	struct rte_ring *rx_ring = rte_ring_lookup(data1.rx_ring_name);
	if (rx_ring == NULL) {
		fprintf(stderr, "Failed to get the RX ring.\n");
		return -1;
	}
	if (rte_ring_enqueue_bulk(rx_ring, (void **)rx_buf, 17, NULL) == 0) {
		fprintf(stderr,
			"Failed to enqueue all packets into the RX ring.\n");
		return -1;
	}

	if (rte_ring_dequeue_bulk(rx_ring, (void **)tx_buf, 17, NULL) == 0) {
		fprintf(stderr,
			"Failed to dequeue all packets into the TX buffer.\n");
		return -1;
	}
	uint16_t tx_count;
	tx_count = rte_eth_tx_burst(munf_manager.tx_port_id, 0, tx_buf, 17);
	if (tx_count != 17) {
		fprintf(stderr,
			"Failed to send(flush) packets to the TX port.\n");
		return -1;
	}

	ffpp_munf_unregister("test_munf_1");
	// Used to test the cleanup.
	struct ffpp_munf_data data2;
	ffpp_munf_register("test_munf_2", &data2);
	ffpp_munf_cleanup_manager(&munf_manager);

	rte_eal_cleanup();
	return 0;
}
