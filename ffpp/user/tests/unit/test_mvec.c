/*
 * test_mvec.c
 */

#include <assert.h>

#include <rte_eal.h>

#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <ffpp/munf.h>
#include <ffpp/mvec.h>

#define NUM_MBUFS 5000

int main(int argc, char *argv[])
{
	// Init EAL environment
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	struct ffpp_munf_manager munf_manager;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_manager(&munf_manager, "test_manager", pool);

	struct rte_mbuf *rx_buf[64];
	uint16_t rx_count;
	uint16_t tx_count;
	uint16_t n = 0;
	struct ffpp_mvec vec;

	ffpp_mvec_init(&vec, 64);

	for (n = 0; n < 17; ++n) {
		rx_count = rte_eth_rx_burst(munf_manager.rx_port_id, 0, rx_buf,
					    64);
		assert(rx_count == 64);
		ffpp_mvec_set_mbufs(&vec, rx_buf, 64);
		ffpp_mvec_print(&vec);
		uint16_t i = 0;
		struct rte_mbuf *m;
		FFPP_MVEC_FOREACH(&vec, i, m)
		{
			assert(rte_pktmbuf_headroom(m) == 128);
		}
		tx_count = rte_eth_tx_burst(munf_manager.tx_port_id, 0, rx_buf,
					    64);
		assert(tx_count == 64);
	}

	ffpp_mvec_free(&vec);

	ffpp_munf_cleanup_manager(&munf_manager);

	rte_eal_cleanup();
	return 0;
}
