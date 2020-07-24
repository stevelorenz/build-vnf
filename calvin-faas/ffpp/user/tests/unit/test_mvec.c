/*
 * test_mvec.c
 */

#include <rte_eal.h>

int main(int argc, char *argv[])
{
	// Init EAL environment
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	rte_eal_cleanup();
	return 0;
}
