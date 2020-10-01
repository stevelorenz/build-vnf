/*
*scaling_defines_user.h
*/

#ifndef SCALING_DEFINES_USER_H
#define SCALING_DEFINES_USER_H

#include <stdbool.h>
#include <math.h>

#include <ffpp/bpf_helpers_user.h>

#define RELEASE 1 // Uncomment to show debug information

// Hyper-parameter for scaling decision
#define INTERVAL 1000 //10000 // Map reading interval during traffic
#define IDLE_INTERVAL 100 // Map reading interval during ISG
#define MAX_EMPTY_CNT 10 //2// So, after two empty polls we sleep :)
#define UTIL_THRESHOLD_UP 0.75 // We don't actually want tu surpass the 0.8
#define UTIL_THRESHOLD_DOWN 0.65
#define HARD_UP_THRESHOLD 0.85
#define HARD_DOWN_THRESHOLD 0.4
#define HARD_INCREASE 5 // Util > @HARD_UP_THRESH -> increase counter up hard
#define HARD_DECREASE 4 // Util < @HARD_DOWN_THRESH -> increase down counter
#define TREND_INCREASE 3 // Increase for up trend
#define TREND_DECREASE 2
#define COUNTER_THRESH 50 //10 // Scale if surpassed
#define NUM_READINGS_SMA 50 //10 // Number of samples for moving averages
#define NUM_READINGS_WMA 25 //5
#define TINTERVAL 1.8 // Confidence interval
#define D_PKT_DOWN_THRESH                                                      \
	ceil(INTERVAL * 1e-6 *                                                 \
	     1200) // Allowed packet count deviation for down scaling
#define D_PKT_UP_THRESH                                                        \
	ceil(INTERVAL * 1e-6 * 2100) // If we surpass this count -> scale up
#define HARD_D_PKT_UP_THRESH ceil(INTERVAL * 1e-6 * 3100)

// System parameter
#define NUM_CORES 8 // Total cores of the system
#define CORE_MASK 2 // Cores to initialize
#define CORE_OFFSET 1 // First core to initialize
#define C_PACKET 506880 //6550 // CPU cycles for one packet
#define MAX_PSTATES 32 // Max possible P-states

#define CPU_UTIL(INTER_TIME, FREQ) (C_PACKET / (INTER_TIME * FREQ))
// #define CPU_FREQ(INTER_TIME) (C_PACKET / (INTER_TIME * UTIL_THRESHOLD_UP))
#define CPU_FREQ(CUR_UTIL, CUR_FREQ) ((CUR_FREQ * CUR_UTIL) / UTIL_THRESHOLD_UP)

struct measurement {
	int lcore;
	unsigned long int cnt;
	int valid_vals; // start new ma's after scale or consecutive zeros
	int min_cnts;
	int idx; //count % min_counts;
	// int scale_down_cnt;
	// int scale_up_cnt;
	int empty_cnt;
	double inter_arrival_time;
	double sma_cpu_util;
	double sma_std_err;
	double wma_cpu_util;
	double cpu_util[NUM_READINGS_SMA];
	// bool up_trend; // false
	// bool down_trend; // false
	bool had_first_packet; // false
};

struct scaling_info {
	double last_scale;
	int scale_down_cnt;
	int scale_up_cnt;
	int empty_cnt;
	unsigned int next_pstate;
	bool up_trend;
	bool down_trend;
	bool scale_to_min;
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

struct feedback_info {
	int packet_offset;
	int delta_packets;
	bool freq_down;
	bool freq_up;
};

struct freq_info {
	unsigned int num_freqs;
	unsigned int pstate;
	unsigned int freq;
	unsigned int freqs[MAX_PSTATES];
};

struct traffic_stats {
	unsigned int total_packets;
	int delta_packets;
	double period;
	double pps;
};

#endif
