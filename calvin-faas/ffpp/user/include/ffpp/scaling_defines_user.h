/*
*scaling_defines_user.h
*/

#ifndef SCALING_DEFINES_USER_H
#define SCALING_DEFINES_USER_H

// Hyper-parameter for scaling decision
#define INTERVAL 2
#define UTIL_THRESHOLD_UP 0.85
#define UTIL_THRESHOLD_DOWN 0.75
#define COUNTER_THRESHOLD 20
#define NUM_READINGS 5

#define NUM_CORES 4
#define C_PACKET 100 // CPU cycles for one packet
#define MAX_PSTATES 32 // Max possible p-states

#define CPU_UTIL(INTER_TIME, FREQ) (C_PACKET / (INTER_TIME * FREQ))
#define CPU_FREQ(INTER_TIME) (C_PACKET / (INTER_TIME * UTIL_THRESHOLD_UP))

struct measurement {
	int lcore;
	__u64 count;
	__u32 min_counts;
	__u32 idx; //count % min_counts;
	__u32 scale_down_count;
	__u32 scale_up_count;
	double inter_arrival_time;
	double avg_cpu_util;
	double cpu_util[NUM_READINGS];
	bool had_first_packet; //false;
};

struct freq_info {
	int num_freqs;
	int pstate;
	int freq;
	unsigned int freqs[MAX_PSTATES];
};

#endif
