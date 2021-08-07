/*
*scaling_defines_user.h
*/

#ifndef SCALING_DEFINES_USER_H
#define SCALING_DEFINES_USER_H

#include <stdbool.h>
#include <math.h>

#include <ffpp/bpf_helpers_user.h>

#ifdef __cplusplus
extern "C" {
#endif

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

// CalcCPU utilization; needs: inter-arrivla time and CPU frequency
#define CPU_UTIL(INTER_TIME, FREQ) (C_PACKET / (INTER_TIME * FREQ))
// Calc new CPU frequency; needs: current CPU util and frequency
#define CPU_FREQ(CUR_UTIL, CUR_FREQ) ((CUR_FREQ * CUR_UTIL) / UTIL_THRESHOLD_UP)

struct measurement {
	int lcore; // Core to scale (obsolte)
	unsigned long int cnt; // Counter for map readings
	int valid_vals; // start new ma's after scale or consecutive zeros
	int min_cnts; // minimum needed counts to start scaling
	int idx; //index for @cpu_util -> count % min_counts;
	// int scale_down_cnt;
	// int scale_up_cnt;
	int empty_cnt; // empty map readings, to detect ISG
	double inter_arrival_time; // current inter-arrival time
	double sma_cpu_util; // simple moving avg over cpu util
	double sma_std_err; // std dev of @sma_cpu_util -> prediction uncertainity
	double wma_cpu_util; // weighted moving avf of cpu util
	double cpu_util[NUM_READINGS_SMA]; // most recent CPU utils
	// bool up_trend; // false
	// bool down_trend; // false
	bool had_first_packet; // indicate when PM becomes active
};

struct scaling_info {
	double last_scale; // TS of last scale
	int scale_down_cnt; // counter for down hints
	int scale_up_cnt; // counter for up hints
	int empty_cnt; // counter forempty readings
	unsigned int next_pstate; // idx of next P-state
	bool up_trend; // up-trend detected
	bool down_trend; // down-trend detected
	bool scale_to_min; // ISG detected
	bool need_scale; // frequency scaling neccessary
	bool scaled_to_min; // scaled tomin during ISG
	bool restore_settings; // first new packet detected
};

// Store values of last stream for a quick wake-up after isg
struct last_stream_settings {
	int last_pstate;
	double last_sma;
	double last_sma_std_err;
	double last_wma;
};

struct feedback_info {
	int packet_offset; // offset -> already lost packets :(
	int delta_packets; // delta btwn in and egress for current reading
	bool freq_down; // all good -> scale down
	bool freq_up; // not so good -> scale up
};

struct freq_info {
	unsigned int num_freqs; // number of available P-states
	unsigned int pstate; // current P-state
	unsigned int freq; // and its frequency value
	unsigned int freqs[MAX_PSTATES]; // index frequency tuples
};

struct traffic_stats {
	unsigned int total_packets; // total received packets
	int delta_packets; // delta btwn in and egress interface
	double period; // time since last reading
	double pps; // ingress pps
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !SCALING_DEFINES_USER_H */
