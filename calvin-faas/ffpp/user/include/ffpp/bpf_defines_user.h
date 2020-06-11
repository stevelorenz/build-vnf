/*
*bpf_defines_user.h
*/

#ifndef BPF_DEFINES_USER_H
#define BPF_DEFINES_USER_H

#include "../../../kern/xdp_fwd/common_kern_user.h"

struct record {
	__u64 timestamp;
	struct datarec total;
};

struct stats_record {
	struct record stats;
};

struct measurement {
	__u64 count;
	__u32 min_counts;
	__u32 idx; //count % min_counts;
	__u32 scale_down_count;
	__u32 scale_up_count;
	double inter_arrival_time;
	double avg_cpu_util;
	double cpu_util[5];
	bool had_first_packet; //false;
};

#endif
