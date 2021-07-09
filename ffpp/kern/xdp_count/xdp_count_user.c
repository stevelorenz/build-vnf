/* SPDX-License-Identifier: GPL-2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <locale.h>
#include <unistd.h>
#include <time.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <net/if.h>
#include <linux/if_link.h> /* depend on kernel-headers installed */

#include "../common/common_defines.h"
#include "../common/ext_xdp_user_utils.h"
#include "common_kern_user.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

const char *pin_basedir = "/sys/fs/bpf/";
const char *map_file_name = "xdp_stats_map";

struct record {
	__u64 timestamp;
	struct datarec total; /* defined in common_kern_user.h */
};

struct stats_record {
	struct record stats[XDP_ACTION_MAX];
};

static double calc_period(struct record *r, struct record *p)
{
	double period_ = 0;
	__u64 period = 0;

	period = r->timestamp - p->timestamp;
	if (period > 0)
		period_ = ((double)period / NANOSEC_PER_SEC);

	return period_;
}

static void stats_print_header()
{
	/* Print stats "header" */
	printf("%-12s\n", "XDP-action");
}

static void stats_print(struct stats_record *stats_rec,
			struct stats_record *stats_prev)
{
	struct record *rec, *prev;
	__u64 packets, bytes;
	double period;
	double pps; /* packets per sec */
	double bps; /* bits per sec */
	int i;

	stats_print_header(); /* Print stats "header" */

	/* Print for each XDP actions stats */
	for (i = 0; i < XDP_ACTION_MAX; i++) {
		char *fmt = "%-12s %'11lld pkts (%'10.0f pps)"
			    " %'11lld Kbytes (%'6.0f Mbits/s)"
			    " period:%f\n";
		const char *action = action2str(i);

		rec = &stats_rec->stats[i];
		prev = &stats_prev->stats[i];

		period = calc_period(rec, prev);
		if (period == 0)
			return;

		packets = rec->total.rx_packets - prev->total.rx_packets;
		pps = packets / period;

		bytes = rec->total.rx_bytes - prev->total.rx_bytes;
		bps = (bytes * 8) / period / 1000000;

		printf(fmt, action, rec->total.rx_packets, pps,
		       rec->total.rx_bytes / 1000, bps, period);
	}
	printf("\n");
}

/* BPF_MAP_TYPE_ARRAY */
void map_get_value_array(int fd, __u32 key, struct datarec *value)
{
	if ((bpf_map_lookup_elem(fd, &key, value)) != 0) {
		fprintf(stderr, "ERR: bpf_map_lookup_elem failed key:0x%X\n",
			key);
	}
}

/* BPF_MAP_TYPE_PERCPU_ARRAY */
void map_get_value_percpu_array(int fd, __u32 key, struct datarec *value)
{
	/* For percpu maps, userspace gets a value per possible CPU */
	unsigned int nr_cpus = libbpf_num_possible_cpus();
	struct datarec values[nr_cpus];
	__u64 sum_bytes = 0;
	__u64 sum_pkts = 0;
	__u32 i;

	if ((bpf_map_lookup_elem(fd, &key, values)) != 0) {
		fprintf(stderr, "ERR: bpf_map_lookup_elem failed key:0x%X\n",
			key);
		return;
	}

	/* Sum values from each CPU */
	for (i = 0; i < nr_cpus; i++) {
		sum_pkts += values[i].rx_packets;
		sum_bytes += values[i].rx_bytes;
	}
	value->rx_packets = sum_pkts;
	value->rx_bytes = sum_bytes;
}

static bool map_collect(int fd, __u32 map_type, __u32 key, struct record *rec)
{
	struct datarec value;

	/* Get time as close as possible to reading map contents */
	rec->timestamp = gettime();

	switch (map_type) {
	case BPF_MAP_TYPE_ARRAY:
		map_get_value_array(fd, key, &value);
		break;
	case BPF_MAP_TYPE_PERCPU_ARRAY:
		map_get_value_percpu_array(fd, key, &value);
		break;
	default:
		fprintf(stderr, "ERR: Unknown map_type(%u) cannot handle\n",
			map_type);
		return false;
		break;
	}

	rec->total.rx_packets = value.rx_packets;
	rec->total.rx_bytes = value.rx_bytes;
	return true;
}

static void stats_collect(int map_fd, __u32 map_type,
			  struct stats_record *stats_rec)
{
	/* Collect all XDP actions stats  */
	__u32 key;

	for (key = 0; key < XDP_ACTION_MAX; key++) {
		map_collect(map_fd, map_type, key, &stats_rec->stats[key]);
	}
}

static void stats_poll(int map_fd, __u32 map_type, int interval)
{
	struct stats_record prev, record = { 0 };

	/* Trick to pretty printf with thousands separators use %' */
	setlocale(LC_NUMERIC, "en_US");

	/* Get initial reading quickly */
	stats_collect(map_fd, map_type, &record);
	usleep(1000000 / 4);

	while (1) {
		prev = record; /* struct copy */
		stats_collect(map_fd, map_type, &record);
		stats_print(&record, &prev);
		sleep(interval);
	}
}

int main(int argc, char **argv)
{
	const struct bpf_map_info map_expect = {
		.key_size = sizeof(__u32),
		.value_size = sizeof(struct datarec),
		.max_entries = XDP_ACTION_MAX,
	};
	char pin_dir[PATH_MAX] = "";
	int stats_map_fd = 0;
	int interval = 2; // seconds.
	struct bpf_map_info info = { 0 };
	int len = 0;
	int err = 0;

	if (argc != 2) {
		fprintf(stderr, "ERR: The device name is not given\n");
		fprintf(stdout, "Usage: ./xdp_count_user ifname\n");
		return EXIT_FAIL_OPTION;
	}
	const char *ifname = argv[1];

	len = snprintf(pin_dir, PATH_MAX, "%s/%s", pin_basedir, ifname);
	if (len < 0) {
		fprintf(stderr, "ERR: creating pin dirname\n");
	}

	printf("%s\n", pin_dir);
	stats_map_fd = open_bpf_map_file(pin_dir, map_file_name, &info);
	if (stats_map_fd < 0) {
		fprintf(stderr, "ERR: Can not open the map file.\n");
		return EXIT_FAIL_BPF;
	}

	err = check_map_fd_info(&info, &map_expect);
	if (err) {
		fprintf(stderr, "ERR: map via FD not compatible\n");
		return err;
	}

	printf("Collecting stats from BPF map\n");

	printf(" - BPF map (bpf_map_type:%d) id:%d name:%s"
	       " key_size:%d value_size:%d max_entries:%d\n",
	       info.type, info.id, info.name, info.key_size, info.value_size,
	       info.max_entries);

	stats_poll(stats_map_fd, info.type, interval);

	return EXIT_OK;
}
