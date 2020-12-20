#include <cassert>

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>

#include "ffpp/munf.h"

int main(int argc, char *argv[])
{
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	struct ffpp_munf_manager munf_manager;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_manager(&munf_manager, "test_manager", pool);

	struct rte_mbuf *rx_buf[3];
	uint16_t rx_count;
	uint16_t tx_count;
	uint16_t n = 0;

	rx_count = rte_eth_rx_burst(munf_manager.rx_port_id, 0, rx_buf, 3);
	assert(rx_count == 3);

	tx_count = rte_eth_tx_burst(munf_manager.tx_port_id, 0, rx_buf, 3);
	assert(tx_count == 3);

	ffpp_munf_cleanup_manager(&munf_manager);
	rte_eal_cleanup();
	return 0;
}
