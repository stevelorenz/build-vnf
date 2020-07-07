#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <time.h>

#include <ffpp/bpf_helpers_user.h>

int open_bpf_map_file(const char *pin_dir, const char *mapname,
		      struct bpf_map_info *info)
{
	char filename[PATH_MAX];
	int err = 0;
	int len = 0;
	int fd = 0;

	uint32_t info_len = sizeof(*info);

	len = snprintf(filename, PATH_MAX, "%s/%s", pin_dir, mapname);
	if (len < 0) {
		fprintf(stderr, "ERR: constructing full mapname path.\n");
		return -1;
	}

	fd = bpf_obj_get(filename);
	if (fd < 0) {
		fprintf(stderr,
			"WARN: Failed to open bpf map file:%s err(%d):%s\n",
			filename, errno, strerror(errno));
		return fd;
	}

	if (info) {
		err = bpf_obj_get_info_by_fd(fd, info, &info_len);
		if (err) {
			fprintf(stderr, "ERR: %s() can't get info - %s\n",
				__func__, strerror(errno));
			return -1;
		}
	}

	return fd;
}

int check_map_fd_info(const struct bpf_map_info *info,
		      const struct bpf_map_info *exp)
{
	if (exp->key_size && exp->key_size != info->key_size) {
		fprintf(stderr,
			"ERR: %s() "
			"Map key size(%d) mismatch expected size(%d)\n",
			__func__, info->key_size, exp->key_size);
		return -1;
	}
	if (exp->value_size && exp->value_size != info->value_size) {
		fprintf(stderr,
			"ERR: %s() "
			"Map value size(%d) mismatch expected size(%d)\n",
			__func__, info->value_size, exp->value_size);
		return -1;
	}
	if (exp->max_entries && exp->max_entries != info->max_entries) {
		fprintf(stderr,
			"ERR: %s() "
			"Map max_entries(%d) mismatch expected size(%d)\n",
			__func__, info->max_entries, exp->max_entries);
		return -1;
	}
	if (exp->type && exp->type != info->type) {
		fprintf(stderr,
			"ERR: %s() "
			"Map type(%d) mismatch expected type(%d)\n",
			__func__, info->type, exp->type);
		return -1;
	}
	return 0;
}

__u64 gettime(void)
{
	struct timespec t;
	int res;

	res = clock_gettime(CLOCK_MONOTONIC, &t);
	if (res < 0) {
		fprintf(stderr, "Error with gettimeofday! (%i)\n", res);
		exit(EXIT_FAIL);
	}

	return (__u64)t.tv_sec * NANOSEC_PER_SEC + t.tv_nsec;
}

void map_get_value_percpu_array(int fd, __u32 key, struct datarec *value)
{
	unsigned int nr_cpus = libbpf_num_possible_cpus();
	struct datarec values[nr_cpus];
	__u64 sum_pkts = 0;
	__u64 latest_time = 0;
	__u32 i;

	if ((bpf_map_lookup_elem(fd, &key, values)) != 0) {
		fprintf(stderr, "ERR:bpf_map_lookup_elem failed key:0x%X\n",
			key);
		return;
	}

	for (i = 0; i < nr_cpus; i++) {
		sum_pkts += values[i].rx_packets;

		// Get the latest time stamp
		if (values[i].rx_time > latest_time) {
			latest_time = values[i].rx_time;
		}
	}

	value->rx_packets = sum_pkts;
	value->rx_time = latest_time;
}

bool map_collect(int fd, __u32 key, struct record *rec)
{
	struct datarec value;

	rec->timestamp = gettime();

	map_get_value_percpu_array(fd, key, &value);

	rec->total.rx_packets = value.rx_packets;
	rec->total.rx_time = value.rx_time;

	return true;
}
