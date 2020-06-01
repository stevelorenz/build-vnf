/*
 * bpf_helpers_user.h
 */

#ifndef BPF_HELPERS_USER_H
#define BPF_HELPERS_USER_H

#include <bpf/bpf.h>

#define EXIT_OK 0
#define EXIT_FAIL 1
#define EXIT_FAIL_OPTION 2
#define EXIT_FAIL_XDP 30
#define EXIT_FAIL_BPF 40

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/**
 * @file
 *
 * BPF helper functions ONLY for userspace programs.
 *
 */

/**
 * Open BPF map file with the given path and map name. The map information
 * variable given as *info is also updated.
 *
 * @param pin_dir
 * @param mapname
 * @param info
 *
 * @return
 *  - The file descriptor of the BPF map file.
 *  - Negative on error.
 */
int open_bpf_map_file(const char *pin_dir, const char *mapname,
		      struct bpf_map_info *info);

/**
 * Check if the given map information matches the format of the expected map.
 *
 * @param info: The empty eBPF map information to be updated.
 * @param exp: The struct of the expected eBPF map.
 *
 * @return
 *  - 0 on success.
 *  - Negative on error.
 */
int check_map_fd_info(const struct bpf_map_info *info,
		      const struct bpf_map_info *exp);

#endif /* !BPF_HELPERS_USER_H */
