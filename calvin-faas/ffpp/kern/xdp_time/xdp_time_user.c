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
	struct datarec total;
};

struct stats_record {
	struct record stats;
};

struct measurement {
	__u64 count;
	__u32 min_counts;
	__u32 idx; //count % min_counts;
	__u32 scale_down_count;
	__u32 scale_up_count;
	double inter_arrival_time;
	double avg_cpu_util;
	double cpu_util[5];
	bool had_first_packet; //false;
};

static double calc_period(struct record *r, struct record *p)
{
	double period_ = 0;
	__u64 period = 0;

	period = r->timestamp - p->timestamp;
	if (period > 0) {
		period_ = ((double)period / NANOSEC_PER_SEC);
	}

	return period_;
}

static double get_cpu_frequency()
{
	FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
	if (cpuinfo == NULL) {
		fprintf(stderr, "ERR: Couldn't get CPU frequency");
		return EXIT_FAILURE;
	}

	const char *key = "MHz";
	char str[255];
	double freq = 0.0;

	while ((fgets(str, sizeof str, cpuinfo)) != NULL) {
		if (strstr(str, key) != NULL) {
			char *s = strtok(str, ":");
			s = strtok(NULL, s); // get second element (frequency)
			freq = atof(s);

			// Currently we get the frequency only for the first core
			break;
		}
	}
	fclose(cpuinfo);

	return freq * 1e3;
}

static void scale_cpu_frequency(struct measurement *measurement)
{
	__u32 c_packet = 100;
	__u32 scale_threshold = 5; // Avoid ping-pong scaling
	double util_threshold[2] = { 0.8, 0.9 };
	double new_cpu_freq;

	if (measurement->avg_cpu_util < util_threshold[0]) {
		measurement->scale_down_count += 1;
		if (measurement->scale_down_count > scale_threshold) {
			// Check which frequency is needed to bring the utilization
			// back up to 80%
			// Use only last inter-arrival time sample or also average?
			measurement->scale_down_count = 0;
			new_cpu_freq = c_packet /
				       (measurement->inter_arrival_time * 0.85);
			printf("Scale CPU frequency down to: %f\n",
			       new_cpu_freq);
		}
	} else if (measurement->avg_cpu_util > util_threshold[1]) {
		measurement->scale_up_count += 1;
		if (measurement->scale_up_count > scale_threshold) {
			// Same check as for down scaling
			measurement->scale_up_count = 0;
			new_cpu_freq = c_packet /
				       (measurement->inter_arrival_time * 0.85);
			printf("Scale CPU frequency up to: %f\n", new_cpu_freq);
		}
	}
}

static void get_cpu_utilization(struct measurement *measurement)
{
	__u32 c_packet = 100; // CPU cycles per processed packet
	int i;
	double sum = 0.0;
	double cpu_freq = get_cpu_frequency();
	measurement->cpu_util[measurement->idx] =
		c_packet / (measurement->inter_arrival_time * cpu_freq);
	if (measurement->count >= measurement->min_counts) {
		for (i = 0; i < measurement->min_counts; i++) {
			sum += measurement->cpu_util[i];
		}
		measurement->avg_cpu_util = sum / measurement->min_counts;

		scale_cpu_frequency(measurement);
	}
	printf("Current CPU utilization: %'.10f, average CPU utilization: %'.10f\n\n",
	       measurement->cpu_util[measurement->idx],
	       measurement->avg_cpu_util);
}

// static void stats_print_header()
// {
// printf("%-12s\n", "XDP-action");
// }

static void stats_print(struct stats_record *stats_rec,
			struct stats_record *stats_prev,
			struct measurement *measurement)
{
	struct record *rec, *prev;
	__u64 packets;
	double inter_arrival;
	double period;
	double pps;

	// stats_print_header();

	char *fmt = "%-12s %'11lld pkts (%'10.0f pps)"
		    // " %'11lld Kbytes (%'6.0f Mbits/s)"
		    " \t%'10.8f s"
		    " \tperiod:%f\n";
	const char *action = action2str(2); // @2: xdp_pass

	rec = &stats_rec->stats;
	prev = &stats_prev->stats;

	period = calc_period(rec, prev);
	if (period == 0) {
		return;
	}

	packets = rec->total.rx_packets - prev->total.rx_packets;
	pps = packets / period;

	// Can we get per-packet measurements in user space?
	// If not: inter_arrival /= packets for an average
	if (packets > 0) {
		inter_arrival = (rec->total.rx_time - prev->total.rx_time) /
				(packets * 1e9);
		if (measurement->had_first_packet) {
			measurement->inter_arrival_time = inter_arrival;
			measurement->count += 1;
			measurement->idx =
				measurement->count % measurement->min_counts;
		} else {
			inter_arrival = 0;
			measurement->had_first_packet = true;
		}
	} else {
		inter_arrival = 0;
	}

	printf(fmt, action, rec->total.rx_packets, pps, inter_arrival, period);
	// printf("\n");
}

void map_get_value_percpu_array(int fd, __u32 key, struct datarec *value)
{
	unsigned int nr_cpus = libbpf_num_possible_cpus();
	struct datarec values[nr_cpus];
	__u64 sum_pkts = 0;
	__u64 latest_time = 0;
	int i;

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

static bool map_collect(int fd, __u32 key, struct record *rec)
{
	struct datarec value;

	rec->timestamp = gettime();

	map_get_value_percpu_array(fd, key, &value);

	rec->total.rx_packets = value.rx_packets;
	rec->total.rx_time = value.rx_time;

	return true;
}

static void stats_collect(int map_fd, struct stats_record *stats_rec)
{
	__u32 key = 0; // Only one entry in our map
	map_collect(map_fd, key, &stats_rec->stats);
}

static void stats_poll(int map_fd, int interval)
{
	struct stats_record prev, record = { 0 };
	struct measurement measurement = { 0 };

	measurement.min_counts = 5;
	measurement.had_first_packet = false;

	setlocale(LC_NUMERIC, "en_US");

	stats_collect(map_fd, &record);
	usleep(1000000 / 4);

	while (1) {
		prev = record;
		stats_collect(map_fd, &record);
		stats_print(&record, &prev, &measurement);
		get_cpu_utilization(&measurement);
		;
		sleep(interval);
	}
}

int main(int argc, char **argv)
{
	const struct bpf_map_info map_expect = {
		.key_size = sizeof(__u32),
		.value_size = sizeof(struct datarec),
		.max_entries = 1,
	};
	char pin_dir[PATH_MAX] = "";
	int stats_map_fd = 0;
	int interval = 2; // seconds
	struct bpf_map_info info = { 0 };
	int len = 0;
	int err = 0;

	if (argc != 2) {
		fprintf(stderr, "ERR: The device name is not given\n");
		fprintf(stdout, "Usage: ./xdp_count_user ifname\n");
		return EXIT_FAIL_OPTION;
	}
	const char *ifname = argv[1];

	len = snprintf(pin_dir, PATH_MAX, "%s%s", pin_basedir, ifname);
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

	double cpu_freq = get_cpu_frequency();
	printf("Frequency: %f\n", cpu_freq);

	stats_poll(stats_map_fd, interval);

	return EXIT_OK;
}
