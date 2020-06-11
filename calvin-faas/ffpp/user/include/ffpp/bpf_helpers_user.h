/*
 * bpf_helpers_user.h
 */

#ifndef BPF_HELPERS_USER_H
#define BPF_HELPERS_USER_H

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <ffpp/bpf_defines_user.h>
#include "../../../kern/xdp_fwd/common_kern_user.h"

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

#define NANOSEC_PER_SEC 1e9
/**
 * @ Get current time stamp in ns
*/
__u64 gettime(void);

/**
 * Get map entries for each core
 * 
 * @param fd: The filedescriptor of the BPG map
 * @param key: Key for the entry to read
 * @param value: The struct to store the read values in
 */
void map_get_value_per_cpu_array(int fd, __u32 key, struct datarec *value);

/**
 * 
 * @param fd: The filedescriptor of the BPG map
 * @param key: Key for the entry to read
 * @param value: The struct to store the read values in
 * 
 * @return 
 * 	- true on succes
 */
bool map_collect(int fd, __u32 key, struct record *rec);

#endif /* !BPF_HELPERS_USER_H */
