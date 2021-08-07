/*
 * general_helpers_user.h
 */

#ifndef GENERAL_HELPERS_USER_H
#define GENERAL_HELPERS_USER_H

#include <linux/bpf.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <rte_power.h>

#include <ffpp/bpf_defines_user.h>
#include <ffpp/scaling_defines_user.h>
#include <ffpp/global_stats_user.h>

#ifdef __cplusplus
extern "C" {
#endif

// Global variables for measurements
// Defined in global_stats_user.h
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
int g_csv_offset[TOTAL_VALS];
unsigned int g_csv_num_val;
int g_csv_num_round;
double cur_time;

/**
 * @brief Writes .csv dump of X-MAN
 * Outile: timestamp, pps, cpu util, cpu frequency
 * Filename: test-"num_round".csv
 * 
 */
void write_csv_file();

/**
 * @brief Writes .csv dump of X-MAN-FB
 * Outile: timestamp, in pps, out pps, delta between in and out,
 * offset between in and out, cpu frequency
 * Filename: fb-"num_round".csv
 * 
 */
void write_csv_file_fb_in();

/**
 * @brief Writes .csv dump of traffic monitor
 * Outile: timestamp, pps, inter-arrival time
 * Filename: test-"num_round".csv
 * 
 */
void write_csv_file_tm();

/**
 * @brief Writes .csv dump of X-MAN for 2 CNF's (per CNF dump)
 * outline: timestamp, cnf pps, cnf cpu util, cpu freq
 * 
 * @param num_vnf vnf id
 */
void write_csv_file_2vnf(int num_vnf);

/**
 * @brief Get the time of day object
 * 
 * @return double current time
 */
double get_time_of_day();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !GENERAL_HELPERS_USER_H */
