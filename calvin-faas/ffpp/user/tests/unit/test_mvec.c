/*
 * test_mvec.c
 */

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

	struct ffpp_munf_manager_ctx munf_manager_ctx;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_manager(&munf_manager_ctx, "test_manager", pool);

	struct rte_mbuf *rx_buf[64];
	uint16_t rx_count;

	rx_count = rte_eth_rx_burst(munf_manager_ctx.rx_port_id, 0, rx_buf, 64);
	assert(rx_count == 64);

	struct ffpp_mvec vec;
	ffpp_mvec_init(&vec, rx_buf, 64);

	ffpp_mvec_print(&vec);
	uint16_t i = 0;
	struct rte_mbuf *m;
	FFPP_MVEC_FOREACH(&vec, i, m)
	{
		assert(rte_pktmbuf_headroom(m) == 128);
	}

	ffpp_mvec_free(&vec);

	ffpp_munf_cleanup_manager(&munf_manager_ctx);

	rte_eal_cleanup();
	return 0;
}
