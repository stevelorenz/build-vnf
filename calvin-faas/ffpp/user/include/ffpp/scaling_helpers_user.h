/*
 * scaling_helpers_user.h
 */

#ifndef SCALING_HELPERS_USER_H
#define SCALING_HELPERS_USER_H

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <rte_power.h>

#include <ffpp/bpf_defines_user.h>
#include <ffpp/scaling_defines_user.h>

/**
 * Obtain number of p-states and respective frequencies of the given lcore
 * 
 * @param lcore: Core for that the info is needed
 * @param f: Struct to store the information
 * @param debug: Prints the obtained information if true
 */
void get_frequency_info(int lcore, struct freq_info *f, bool debug);

/**
 * Reads the current frequency of the given core from /proc/cpuinfo
 * 
 * @param lcore: core id for the core of interest
 * 
 * @return
 *  - Frequency for the given core
 */
double get_cpu_frequency(int lcore);

/**
 * Calculates the time between two map readings
 * 
 * @param r: struct of the current reading
 * @param p: struct of the previous reading
 * 
 * @return
 *  - user space time between current and previous reading
 */
double calc_period(struct record *r, struct record *p);

/**
 * Sends CPU to the given p-state
 * 
 * @param f: struct with frequency information of the CPU
 * @param pstate: pstate the CPU shall enter
 */
void set_pstate(struct freq_info *f, int pstate);

/**
 * Enables Turbo Boost for the CPU
 */
void set_turbo();

/**
 * Calculates the p-state that is required for the current inter-arrival
 * time and CPU utilization threshold
 * 
 * @param m: struct of the current measurement status and values
 * @param f: struct with frequency information of the CPU
 * 
 * @return
 *  - p-state index
 */
int calc_pstate(struct measurement *m, struct freq_info *f);

/**
 * Checks the current CPU utilization and increnebts the 
 * respective counter
 * 
 * @param m: struct of the current measurement status and values
 * @param f: struct with frequency information of the CPU
 */
void check_frequency_scaling(struct measurement *m, struct freq_info *f);

/**
 * Calculates the CPU utilization according to the recent traffic
 * 
 * @param m: struct of the current measurement status and values
 * @param f: struct with frequency information of the CPU
 */
void get_cpu_utilization(struct measurement *m, struct freq_info *f);

#endif /* !BPF_SCALING_USER_H */
