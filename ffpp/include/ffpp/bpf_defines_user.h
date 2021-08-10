/*
 * bpf_defines_user.h
 */

#ifndef BPF_DEFINES_USER_H
#define BPF_DEFINES_USER_H

#include <linux/if_ether.h>
#include <net/if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data record stored in the map.
 */
struct datarec {
	__u64 rx_packets;
	__u64 rx_time;
};

// Naming ref: man ovs-fields.
// ISSUE: The match filed CAN NOT be a struct.
// struct match_fileds {
// 	__u8 eth_src[ETH_ALEN];
// 	__u16 udp_src;
// };

struct fwd_params {
	__u8 eth_src[ETH_ALEN];
	__u8 eth_dst[ETH_ALEN];
	__u8 eth_new_src[ETH_ALEN];
	__u8 eth_new_dst[ETH_ALEN];
	char redirect_ifname_buf[IF_NAMESIZE];
};

struct record {
	__u64 timestamp;
	struct datarec total;
};

struct stats_record {
	struct record stats;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
