/*
*scaling_defines_user.h
*/

#ifndef SCALING_DEFINES_USER_H
#define SCALING_DEFINES_USER_H

#include <stdbool.h>

#include <ffpp/bpf_helpers_user.h>

// Hyper-parameter for scaling decision
#define INTERVAL 100000
#define MAX_EMPTY_CNT 1 // So, after two empty polls we sleep :)
#define UTIL_THRESHOLD_UP 0.80
#define UTIL_THRESHOLD_DOWN 0.70
#define HARD_UP_THRESHOLD 0.90
#define HARD_DOWN_THRESHOLD 0.4
#define HARD_INCREASE 5
#define HARD_DECREASE 4
#define TREND_INCREASE 3
#define TREND_DECREASE 2
#define COUNTER_THRESHOLD 10
#define NUM_READINGS_SMA 10
#define NUM_READINGS_WMA 5
#define TINTERVAL 1.8

#define NUM_CORES 8 // Total cores of the system
#define CORE_MASK 2 // Cores to initialize
#define CORE_OFFSET 1 // First core to initialize
#define C_PACKET 27650 //3150 // CPU cycles for one packet
#define MAX_PSTATES 32 // Max possible p-states

#define CPU_UTIL(INTER_TIME, FREQ) (C_PACKET / (INTER_TIME * FREQ))
// #define CPU_FREQ(INTER_TIME) (C_PACKET / (INTER_TIME * UTIL_THRESHOLD_UP))
#define CPU_FREQ(CUR_UTIL, CUR_FREQ) ((CUR_FREQ * CUR_UTIL) / UTIL_THRESHOLD_UP)

struct measurement {
	int lcore;
	unsigned long int cnt;
	int valid_vals; // start new ma's after scale or consecutive zeros
	int min_cnts;
	int idx; //count % min_counts;
	int scale_down_cnt;
	int scale_up_cnt;
	int empty_cnt;
	double inter_arrival_time;
	double sma_cpu_util;
	double sma_std_err;
	double wma_cpu_util;
	double cpu_util[NUM_READINGS_SMA];
	bool up_trend; // false
	bool down_trend; // false
	bool had_first_packet; // false
};

struct scaling_info {
	double last_scale;
	int scale_down_cnt;
	int scale_up_cnt;
	int empty_cnt;
	int next_pstate;
	bool up_trend;
	bool down_trend;
	bool scale_min;
	bool need_scale;
	bool scaled_to_min;
	bool restore_settings;
};

// Store values of last stream for a quick wake-up after isg
struct last_stream_settings {
	int last_pstate;
	double last_sma;
	double last_sma_std_err;
	double last_wma;
};

struct freq_info {
	int num_freqs;
	int pstate;
	int freq;
	unsigned int freqs[MAX_PSTATES];
};

struct traffic_stats {
	unsigned int packets;
	double period;
	double pps;
};

#endif
