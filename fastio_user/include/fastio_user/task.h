/*
 * task.h
 */

#ifndef TASK_H
#define TASK_H

/**
 * @file
 *
 * Tasks/Jobs and Workers management
 *
 */

#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_memory.h>

#include "device.h"

/**
 * print_lcore_infos() - Print all lcores' information
 */
void print_lcore_infos(void);

/**
 * @brief dpdk_enter_mainloop_master
 *
 * @param func
 */
void dpdk_enter_mainloop_master(lcore_function_t* func, void* args);

/**
 * struct worker - A worker for processing received packets.
 *
 * Each worker is launched on a slave lcore and currently handles ONLY one RX
 * and TX queue of a port. E.g. for 2 ports with 2 queues per port, worker 0
 * handle rx/tx queue 0 of port 0 and 1, and worker 1 handle rx/tx queue 1 of
 * port 0 and 1. This dispatch focuses on performance, multiple queues are
 * handled by different lcores even for single port
 *
 */
struct worker {
        struct rx_queue rxqs[FASTIO_USER_MAX_PORTS];
        struct tx_queue txqs[FASTIO_USER_MAX_PORTS];
        uint16_t core_id;
} __rte_cache_aligned;

/**
 * launch_workers() - Launch the same function on given workers.
 *
 * @param func
 * @param workers
 *
 * @return
 */
int launch_workers(lcore_function_t* func, struct worker* workers);

#endif /* !TASK_H */
