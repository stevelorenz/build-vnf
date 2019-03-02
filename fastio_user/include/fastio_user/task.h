/*
 * task.h
 */

#ifndef TASK_H
#define TASK_H

#include <rte_launch.h>
#include <rte_lcore.h>

/**
 * @brief dpdk_enter_mainloop_master
 *
 * @param func
 */
void dpdk_enter_mainloop_master(lcore_function_t* func);

#endif /* !TASK_H */
