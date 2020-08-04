/*
 * general_helpers_user.h
 */

#ifndef GENERAL_HELPERS_USER_H
#define GENERAL_HELPERS_USER_H

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <rte_power.h>

#include <ffpp/bpf_defines_user.h>
#include <ffpp/scaling_defines_user.h>
#include <ffpp/global_stats_user.h>

double g_csv_pps[TOTAL_VALS];
double g_csv_ts[TOTAL_VALS];
double g_csv_iat[TOTAL_VALS];
double g_csv_cpu_util[TOTAL_VALS];
unsigned int g_csv_freq[TOTAL_VALS];
unsigned int g_csv_num_val;
int g_csv_num_round;
double cur_time;

void write_csv_file();

double get_time_of_day();

#endif /* !GENERAL_HELPERS_USER_H */
