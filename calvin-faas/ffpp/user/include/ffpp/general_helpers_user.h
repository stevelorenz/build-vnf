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
double g_csv_pps_mult[NUM_VNFS][TOTAL_VALS];
double g_csv_ts[TOTAL_VALS];
double g_csv_iat[TOTAL_VALS];
double g_csv_cpu_util[TOTAL_VALS];
double g_csv_cpu_util_mult[NUM_VNFS][TOTAL_VALS];
unsigned int g_csv_freq[TOTAL_VALS];
double g_csv_in_pps[TOTAL_VALS];
double g_csv_out_pps[TOTAL_VALS];
int g_csv_out_delta[TOTAL_VALS];
unsigned int g_csv_num_val;
int g_csv_num_round;
double cur_time;

void write_csv_file();

void write_csv_file_fb_in();

void write_csv_file_fb_out();

void write_csv_file_tm();

void write_csv_file_2vnf(int num_vnf);

double get_time_of_day();

#endif /* !GENERAL_HELPERS_USER_H */
