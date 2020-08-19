/*
 * About: A cool power manager that manage the CPU frequency based on the
 * statistics provided by the XDP program(s).
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <locale.h>
#include <cpufreq.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_power.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_timer.h>

#include <ffpp/bpf_helpers_user.h>
#include <ffpp/scaling_helpers_user.h>
#include <ffpp/bpf_defines_user.h>
#include <ffpp/scaling_defines_user.h>
#include <ffpp/general_helpers_user.h>
#include <ffpp/global_stats_user.h>
#include "../../../kern/xdp_fwd/common_kern_user.h"

#define RELEASE 1
#ifdef RELEASE
#define printf(fmt, ...) (0)
#endif

static volatile bool force_quit;

double g_csv_pps[TOTAL_VALS];
double g_csv_ts[TOTAL_VALS];
double g_csv_iat[TOTAL_VALS];
double g_csv_cpu_util[TOTAL_VALS];
unsigned int g_csv_freq[TOTAL_VALS];
unsigned int g_csv_num_val = 0;
int g_csv_num_round = 0;
int g_csv_empty_cnt = 0;
int g_csv_empty_cnt_threshold = 50; // Calc depending on interval
bool g_csv_saved_stream = false;

const char *pin_basedir = "/sys/fs/bpf";

static void signal_handler(int signum)
{
	if (signum == SIGINT || SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
		       signum);
		force_quit = true;
	}
}

static void stats_print(struct stats_record *stats_rec,
			struct stats_record *stats_prev, struct measurement *m,
			struct scaling_info *si)
{
	struct record *rec, *prev;
	struct traffic_stats t_s = { 0 };

	char *fmt = // "%-12s"
		"%d"
		"%'11lld pkts (%'10.0f pps)"
		" \t%'10.8f s"
		" \tperiod:%f\n";
	// const char *action = action2str(2); // @2: xdp_pass

	rec = &stats_rec->stats;
	prev = &stats_prev->stats;

	calc_traffic_stats(m, rec, prev, &t_s, si);

	// Collect stats in global buffers
	if (t_s.delta_packets > 0) {
		if (m->had_first_packet) {
			// Collect stats
			g_csv_ts[g_csv_num_val] = get_time_of_day();
			g_csv_pps[g_csv_num_val] = t_s.pps;
			g_csv_iat[g_csv_num_val] = m->inter_arrival_time;
			g_csv_num_val++;
			g_csv_empty_cnt = 0;
		}
	} else if (t_s.delta_packets == 0 && m->had_first_packet) {
		if (!g_csv_saved_stream) {
			g_csv_ts[g_csv_num_val] = get_time_of_day();
			g_csv_pps[g_csv_num_val] = t_s.pps;
			g_csv_iat[g_csv_num_val] = m->inter_arrival_time;
			g_csv_num_val++;
			g_csv_empty_cnt++;
		}
		if (g_csv_empty_cnt >= g_csv_empty_cnt_threshold) {
			write_csv_file_tm();
			g_csv_saved_stream = true;
			g_csv_empty_cnt = 0;
			g_csv_num_val = 0;
			g_csv_num_round++;
			si->scaled_to_min = false;
			m->had_first_packet = false;
		}
	}

	printf(fmt, //action,
	       m->cnt, rec->total.rx_packets, t_s.pps, m->inter_arrival_time,
	       t_s.period);
}

static void stats_collect(int map_fd, struct stats_record *stats_rec)
{
	__u32 key = 0; // Only one entry in our map
	map_collect(map_fd, key, &stats_rec->stats);
}

static void stats_poll(int map_fd)
{
	struct stats_record prev, record = { 0 };
	struct measurement m = { 0 };
	struct scaling_info si = { 0 };
	struct last_stream_settings lss = { 0 };

	m.had_first_packet = false;
	m.min_cnts = NUM_READINGS_SMA;

	setlocale(LC_NUMERIC, "en_US");
	stats_collect(map_fd, &record);
	usleep(1000000 / 4);

	while (!force_quit) {
		prev = record;
		stats_collect(map_fd, &record);
		stats_print(&record, &prev, &m, &si);
		printf("\n");
		usleep(INTERVAL);
	}
}

int main(int argc, char *argv[])
{
	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	struct bpf_map_info xdp_stats_map_info = { 0 };
	const struct bpf_map_info xdp_stats_map_expect = {
		.key_size = sizeof(__u32),
		.value_size = sizeof(struct datarec),
		.max_entries = 1,
	};

	char pin_dir[PATH_MAX] = "";
	int xdp_stats_map_fd = 0;

	// MARK: Currently hard-coded to avoid conflicts with DPDK's CLI parser.
	// const char *ifname = "eno2";
	const char *ifname = argv[1];

	int len = 0;
	len = snprintf(pin_dir, PATH_MAX, "%s/%s", pin_basedir, ifname);
	if (len < 0) {
		fprintf(stderr, "ERR: creating pin dirname\n");
	}

	// Open xdp_stats_map from @ifname interface
	int err = 0;
	xdp_stats_map_fd = open_bpf_map_file(pin_dir, "xdp_stats_map",
					     &xdp_stats_map_info);
	if (xdp_stats_map_fd < 0) {
		fprintf(stderr, "ERR: Can not open the XDP stats map file.\n");
		return EXIT_FAIL_BPF;
	}

	err = check_map_fd_info(&xdp_stats_map_info, &xdp_stats_map_expect);
	if (err) {
		fprintf(stderr, "ERR: XDP stats map via FD not compatible.\n");
		return err;
	}
	printf("Successfully open the map file of xdp stats!\n");

	// Print stats from xdp_stats_map
	printf("Collecting stats from BPF map:\n");
	stats_poll(xdp_stats_map_fd);

	/// Save global stats here --> less signaling between single sessions
	/// Get PID with ffpp_power and the simply kill PID
	printf("\nBye..\n");
	return 0;
}
