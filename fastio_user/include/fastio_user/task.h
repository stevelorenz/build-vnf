/*
 * task.h
 */

#ifndef TASK_H
#define TASK_H

/**
 * @file
 *
 * Tasks/Jobs management
 *
 */

#include <rte_launch.h>
#include <rte_lcore.h>

void print_lcore_infos(void);

/**
 * @brief dpdk_enter_mainloop_master
 *
 * @param func
 */
void dpdk_enter_mainloop_master(lcore_function_t* func, void* args);

#endif /* !TASK_H */
