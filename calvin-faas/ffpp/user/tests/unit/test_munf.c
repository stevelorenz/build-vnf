#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_mempool.h>

#include <ffpp/munf.h>

int main(int argc, char *argv[])
{
	ffpp_munf_eal_init(argc, argv);

	struct munf_ctx_t ctx;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_primary(&ctx, "test_primary_process", pool);

	ffpp_munf_cleanup_primary(&ctx);
	return 0;
}
