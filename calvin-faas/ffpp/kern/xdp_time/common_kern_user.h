/*
 * This header file is used by both kernel side BPF-progs and userspace
 * programs. For sharing common structs and DEFINEs.
 */

#ifndef __COMMON_KERN_USER_H
#define __COMMON_KERN_USER_H

/**
 * @brief Data record stored in the map.
 */
struct datarec {
	__u64 rx_packets;
	// __u64 rx_bytes;
	__u64 rx_time;
};

#ifndef XDP_ACTION_MAX
#define XDP_ACTION_MAX (XDP_REDIRECT + 1)
#endif

#endif /* __COMMON_KERN_USER_H */
