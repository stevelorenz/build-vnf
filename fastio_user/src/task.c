/*
 * task.c
 */

#include <rte_common.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>

#include "device.h"
#include "task.h"

void print_lcore_infos(void)
{
        uint16_t i;

        printf("*** Print the lcores information:\n");
        printf("- The ID of the master lcore: %u\n", rte_get_master_lcore());
        printf("%s", "- The ID of the slave lcores: ");
        RTE_LCORE_FOREACH_SLAVE(i) { printf("%u,", i); }
        printf("\n");
}

void dpdk_enter_mainloop_master(lcore_function_t* func, void* args)
{
        RTE_LOG(INFO, FASTIO_USER,
            "Enter main loop for the master lcore with ID: %u\n",
            rte_lcore_id());
        func(args);

        /* Wait until all lcores finish their jobs */
        rte_eal_mp_wait_lcore();

        RTE_LOG(INFO, FASTIO_USER, "Exit main loop. Run cleanups.\n");
        dpdk_cleanup_devices();

        /* MARK: Add cleanups for master core */

        /* TODO: Release memory resources if this is fully supported by the
         * upstream  <01-03-19, Zuo> */
}
