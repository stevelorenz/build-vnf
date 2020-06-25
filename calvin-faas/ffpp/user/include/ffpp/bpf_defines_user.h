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

#endif
