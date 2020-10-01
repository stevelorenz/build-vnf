/*
 * task.c
 */

#include <rte_common.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>

#include <ffpp/config.h>
#include <ffpp/device.h>
#include <ffpp/task.h>

void print_lcore_infos(void)
{
	uint16_t i;

	printf("*** Print the lcores information:\n");
	printf("- The ID of the master lcore: %u\n", rte_get_master_lcore());
	printf("%s", "- The ID of the slave lcores: ");
	RTE_LCORE_FOREACH_SLAVE(i)
	{
		printf("%u,", i);
	}
	printf("\n");
}

void dpdk_enter_mainloop_master(lcore_function_t *func, void *args)
{
	RTE_LOG(INFO, FFPP,
		"Enter main loop for the master lcore with ID: %u\n",
		rte_lcore_id());
	if (func != NULL) {
		func(args);
	}
	/* Wait until all lcores finish their jobs */
	rte_eal_mp_wait_lcore();

	RTE_LOG(INFO, FFPP, "Exit main loop. Run cleanups.\n");
	ffpp_dpdk_cleanup_devices();

	/* MARK: Add cleanups for master core */

	/* TODO: Release memory resources if this is fully supported by the
         * upstream  <01-03-19, Zuo> */
}

int launch_workers(lcore_function_t *func, struct worker *workers)
{
	int core;
	RTE_LCORE_FOREACH_SLAVE(core)
	{
		int ret;
		ret = rte_eal_remote_launch(func, &workers[core], core);
		if (ret < 0) {
			RTE_LOG(ERR, FFPP, "Cannot launch worker for core:%d\n",
				core);
		}
	}
	return 0;
}
