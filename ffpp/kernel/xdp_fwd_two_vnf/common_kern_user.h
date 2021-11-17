/*
 * This header file is used by both kernel side BPF-progs and userspace
 * programs. For sharing common structs and DEFINEs.
 */

#ifndef __COMMON_KERN_USER_H
#define __COMMON_KERN_USER_H

#include <linux/if_ether.h>
#include <net/if.h>

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

#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_REDIRECT + 1)
#endif

#endif /* __COMMON_KERN_USER_H */
