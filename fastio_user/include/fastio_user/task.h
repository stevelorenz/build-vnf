/*
 * task.h
 */

#ifndef TASK_H
#define TASK_H

#include <rte_launch.h>
#include <rte_lcore.h>

#define RTE_LOGTYPE_FASTIO_USER RTE_LOGTYPE_USER1

void print_lcore_infos(void);

/**
 * @brief dpdk_enter_mainloop_master
 *
 * @param func
 */
void dpdk_enter_mainloop_master(lcore_function_t* func, void* args);

#endif /* !TASK_H */
