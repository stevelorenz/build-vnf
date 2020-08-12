/*
 * This header file is used by both kernel side BPF-progs and userspace
 * programs. For sharing common structs and DEFINEs.
 */

#ifndef __COMMON_KERN_USER_H
#define __COMMON_KERN_USER_H

#include <linux/if_ether.h>

/**
 * @brief Data record stored in the map.
 */
struct datarec {
	__u64 rx_packets;
	__u64 rx_time;
};

// Naming ref: man ovs-fields.
struct match_fileds {
	__u8 eth_src[ETH_ALEN];
	__u16 udp_src;
};

struct action_fields {
	__u8 eth_dst[ETH_ALEN];
	__u8 eth_new_src[ETH_ALEN];
};

#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_REDIRECT + 1)
#endif

#endif /* __COMMON_KERN_USER_H */
