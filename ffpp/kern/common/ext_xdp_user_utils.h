/* SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-2-clause)
 * 
 * Extra/additional BPF/XDP functions used by userspace side programs.
 * */

#ifndef __EXT_XDP_USER_UTILS
#define __EXT_XDP_USER_UTILS

int xdp_link_attach(int ifindex, __u32 xdp_flags, int prog_fd);
int xdp_link_detach(int ifindex, __u32 xdp_flags, __u32 expected_prog_id);

struct bpf_object *load_bpf_object_file(const char *filename, int ifindex);
struct bpf_object *load_bpf_and_xdp_attach(struct config *cfg);

const char *action2str(__u32 action);

int check_map_fd_info(const struct bpf_map_info *info,
		      const struct bpf_map_info *exp);

int open_bpf_map_file(const char *pin_dir, const char *mapname,
		      struct bpf_map_info *info);

#define NANOSEC_PER_SEC 1000000000 /* 10^9 */
/**
 * @ Get current time stamp in nanoseconds.
 */
__u64 gettime(void);

/**
 * @ Set the memlock limitation to unlimited.
 *
 * @return
 * - 0: upon successful completion.
 *- -1: error.
 */
int ulimit_memlock_rlimit(void);

#endif /* __EXT_XDP_USER_UTILS */
