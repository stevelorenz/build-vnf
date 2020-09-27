#include <rte_eal.h>

#include <rte_cycles.h>
#include <rte_mempool.h>

#include <ffpp/munf.h>

int main(int argc, char *argv[])
{
	if (rte_eal_init(argc, argv) < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	struct ffpp_munf_manager_ctx munf_manager_ctx;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_manager(&munf_manager_ctx, "test_manager", pool);
	ffpp_munf_register("test_munf_1");
	ffpp_munf_unregister("test_munf_1");

	ffpp_munf_cleanup_manager(&munf_manager_ctx);

	rte_eal_cleanup();
	return 0;
}
