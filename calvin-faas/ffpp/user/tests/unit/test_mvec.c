/*
 * test_mvec.c
 */

#include <rte_eal.h>

#include <ffpp/memory.h>
#include <ffpp/mvec.h>

#define NUM_MBUFS 5000

int main(int argc, char *argv[])
{
	// Init EAL environment
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	struct rte_mempool *pool;
	pool = ffpp_init_mempool("test_mvec_pool", NUM_MBUFS, rte_socket_id(),
				 RTE_MBUF_DEFAULT_BUF_SIZE);

	struct mvec *vec;
	vec = mvec_new();
	mvec_free(vec);

	rte_mempool_free(pool);

	rte_eal_cleanup();
	return 0;
}
