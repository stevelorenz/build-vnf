/*
 * task.c
 */

#include <rte_common.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>

#include "device.h"
#include "task.h"

void dpdk_enter_mainloop_master(lcore_function_t* func)
{
        uint16_t lcore_id;

        RTE_LOG(INFO, SCHED, "Enter main loop for the master lcore.\n");
        rte_eal_mp_remote_launch(func, NULL, CALL_MASTER);

        RTE_LCORE_FOREACH_SLAVE(lcore_id)
        {
                if (rte_eal_wait_lcore(lcore_id) < 0) {
                        break;
                }
        }

        RTE_LOG(INFO, SCHED, "Exit main loop. Run cleanups.\n");
        dpdk_cleanup_devices();

        /* TODO: Release memory resources if this is fully supported by the
         * upstream  <01-03-19, Zuo> */
}
