/*
 * About: A cool power manager that manage the CPU frequency based on 
 * feedback from the CNF egress interface. Scaling is based upon AIMD-algorithm
 * All stats are provided by fancy XDP traffic monitors
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <locale.h>
#include <cpufreq.h>
#include <zmq.h>
#include <jansson.h>

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
#include <math.h>

// supress prints
#ifdef RELEASE
#define printf(fmt, ...) (0)
#endif

static volatile bool force_quit;

// Gloabl measuremnt arrays
double g_csv_in_pps[TOTAL_VALS];
double g_csv_out_pps[TOTAL_VALS];
double g_csv_ts[TOTAL_VALS];
unsigned int g_csv_freq[TOTAL_VALS];
int g_csv_out_delta[TOTAL_VALS];
int g_csv_offset[TOTAL_VALS];
unsigned int g_csv_num_val = 0;
int g_csv_num_round = 0;
int g_csv_empty_cnt = 0;
int g_csv_empty_cnt_threshold = (3 * 1e6) / IDLE_INTERVAL;
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

static int init_power_library(void)
{
	int ret = 0;
	int lcore_id = 0;
	for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	     lcore_id += CORE_MASK) {
		ret = rte_power_init(lcore_id);
		if (ret) {
			RTE_LOG(ERR, POWER,
				"Can not init power library on core: %u\n",
				lcore_id);
		}
	}
	return ret;
}

// Core of the OS and XDP
static int init_power_library_on_system(void)
{
	int ret = 0;
	int lcore_id = 0;
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id += CORE_MASK) {
		ret = rte_power_init(lcore_id);
		if (ret) {
			RTE_LOG(ERR, POWER,
				"Can't init power library on core: %u.\n",
				lcore_id);
		}
	}
	return ret;
}

static int exit_power_library(void)
{
	int ret = 0;
	int lcore_id = 0;
	for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	     lcore_id += CORE_MASK) {
		if (rte_lcore_is_enabled(lcore_id)) {
			ret = rte_power_exit(lcore_id);
			if (ret)
				RTE_LOG(ERR, POWER,
					"Library exit failed on core %u\n",
					lcore_id);
		}
	}
	return ret;
}

static int exit_power_library_on_system(void)
{
	int ret = 0;
	int lcore_id = 0;
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id += CORE_MASK) {
		if (rte_lcore_is_enabled(lcore_id)) {
			ret = rte_power_exit(lcore_id);
			if (ret)
				RTE_LOG(ERR, POWER,
					"Library exit failed on core %u\n",
					lcore_id);
		}
	}
	return ret;
}

static void check_lcore_power_caps(void)
{
	int ret = 0;
	int lcore_id = 0;
	int cnt = 0;
	for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	     lcore_id += CORE_MASK) {
		struct rte_power_core_capabilities caps;
		ret = rte_power_get_capabilities(lcore_id, &caps);
		if (ret == 0) {
			RTE_LOG(INFO, POWER, "Lcore:%d has power capability.\n",
				lcore_id);
			cnt += 1;
		}
	}
	if (cnt == 0) {
		rte_exit(
			EXIT_FAILURE,
			"None of the enabled lcores has the power capability.");
	}
}

static void
collect_global_stats(struct traffic_stats *ts, struct freq_info *f,
		     struct feedback_info *fb, struct measurement *m,
		     __attribute__((unused)) struct scaling_info *si)
{
	if (m->had_first_packet) {
		g_csv_ts[g_csv_num_val] = get_time_of_day();
		// Ingress
		g_csv_in_pps[g_csv_num_val] = ts[0].pps;
		g_csv_freq[g_csv_num_val] = f->freq;
		// Egress
		g_csv_out_pps[g_csv_num_val] = ts[1].pps;
		g_csv_out_delta[g_csv_num_val] = fb->delta_packets;
		g_csv_offset[g_csv_num_val] = fb->packet_offset;
		g_csv_num_val++;
		if (ts[0].delta_packets > 0) {
			g_csv_empty_cnt = 0;
		} else {
			if (!g_csv_saved_stream) {
				g_csv_empty_cnt += 1;
			}
			if (g_csv_empty_cnt > g_csv_empty_cnt_threshold) {
				write_csv_file_fb_in();
				g_csv_saved_stream = true;
				g_csv_empty_cnt = 0;
				g_csv_num_val = 0;
				g_csv_num_round++;
				// si->scaled_to_min = false;
				m->had_first_packet = false;
			}
		}
	}
}

static void stats_print(struct stats_record *stats_rec,
			struct stats_record *stats_prev, struct measurement *m,
			struct scaling_info *si, struct traffic_stats *t_s)
{
	struct record *rec, *prev;
	rec = &stats_rec->stats;
	prev = &stats_prev->stats;

	calc_traffic_stats(m, rec, prev, t_s, si);

	printf("%ld %11lld pkts (%10.0f pps) \t%10.8f s \tperiod:%f\n", m->cnt,
	       rec->total.rx_packets, t_s->pps, m->inter_arrival_time,
	       t_s->period);
}

static void print_feedback(__attribute__((unused)) int count,
			   __attribute__((unused)) struct traffic_stats *ts)
{
	printf("%d %11d pkts (%10.0f pps) \t\t\tperiod:%f\n", count,
	       ts->total_packets, ts->pps, ts->period);
}

static void stats_collect(int map_fd, struct stats_record *stats_rec)
{
	__u32 key = 0; // Only one entry in our map
	map_collect(map_fd, key, &stats_rec->stats);
}

static void stats_poll(int *stats_map_fd, struct freq_info *freq_info)
{
	/// @2 -> ingress and egress map -> Put macro!!
	int i;
	struct stats_record prev[2], record[2] = { 0 };
	struct measurement m = { 0 };
	struct feedback_info fb = { 0 };
	struct traffic_stats ts[2] = { 0 };
	struct scaling_info si = { 0 };
	struct last_stream_settings lss = { 0 };

	m.lcore = rte_lcore_id(); // obsolet
	m.min_cnts = NUM_READINGS_SMA;
	m.had_first_packet = false;

	setlocale(LC_NUMERIC, "en_US");
	for (i = 0; i < 2; i++) {
		stats_collect(stats_map_fd[i], &record[i]);
	}
	fb.packet_offset = record[0].stats.total.rx_packets -
			   record[1].stats.total.rx_packets;
	printf("Initial packet offset %d\n", fb.packet_offset);
	usleep(1000000 / 4);

	while (!force_quit) {
		/// Do the full thing only for the ingress
		/// Egress is just total packets (and pps)
		for (i = 0; i < 2; i++) {
			/// TEST: Put a small break between the reading of the two
			/// interfaces -> reduce difference
			// if (i == 0) {
			// usleep(floor(10000)); // Avg. vnf latency
			// }
			prev[i] = record[i];
			stats_collect(stats_map_fd[i], &record[i]);
		}
		stats_print(&record[0], &prev[0], &m, &si, &ts[0]);
		get_feedback_stats(ts, &fb, &record[1].stats, &prev[1].stats);
		print_feedback(m.cnt, &ts[1]);
		si.empty_cnt = m.empty_cnt;

		collect_global_stats(ts, freq_info, &fb, &m, &si);

		if (si.restore_settings) {
			restore_last_stream_settings(&lss, freq_info, &si);
		}
		/// Go here directly after ISG? -> We do :)
		if (m.valid_vals > 0) {
			check_feedback(&fb, &si);
			if (si.scale_to_min) {
				// Store settings of last stream
				// @1: highest frequency P-state
				lss.last_pstate = 1; //freq_info->pstate;
				if (freq_info->pstate !=
				    (freq_info->num_freqs - 1)) {
					printf("Scale due to empty polls\n");
					si.next_pstate =
						freq_info->num_freqs - 1,
					set_pstate(freq_info, &si);
				} else {
					printf("Already at min\n");
				}
				// Reset flags and counters
				si.scale_to_min = false;
				si.scaled_to_min = true;
				si.need_scale = false;
				m.valid_vals = 0;
				fb.freq_down = false;
				fb.freq_up = false;
				si.scale_down_cnt = 0;
				si.scale_up_cnt = 0;
			} else if (fb.freq_down) {
				si.next_pstate = freq_info->pstate + 1;
				set_pstate(freq_info, &si);
				fb.freq_down = false;
				si.scale_down_cnt = 0;
			} else if (fb.freq_up) {
				si.next_pstate = floor(freq_info->pstate / 2);
				set_pstate(freq_info, &si);
				fb.freq_up = false;
				si.scale_up_cnt = 0;
			}
			fb.packet_offset =
				ts[0].total_packets - ts[1].total_packets;
			printf("Packet offset %d\n", fb.packet_offset);
			printf("CPU frequency %d kHz\n", freq_info->freq);
			usleep(INTERVAL);
		} else {
			usleep(IDLE_INTERVAL);
		}
		set_system_pstate(1);
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr,
			"Please supply ingress and egress interface names, resepectively");
		return -1;
	}
	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	struct bpf_map_info xdp_stats_map_info[2] = { 0 };
	const struct bpf_map_info xdp_stats_map_expect = {
		.key_size = sizeof(__u32),
		.value_size = sizeof(struct datarec),
		.max_entries = 1,
	};

	char pin_dir[PATH_MAX] = "";
	int xdp_stats_map_fd[2] = { 0 };
	int i;
	int len;
	int err;
	const char *ifname = NULL;
	for (i = 0; i < 2; i++) {
		memset(&pin_dir[0], 0, sizeof(pin_dir));
		ifname = argv[i + 1];
		printf("Interface %d: %s\n", i, ifname);
		len = snprintf(pin_dir, PATH_MAX, "%s/%s", pin_basedir, ifname);
		if (len < 0) {
			fprintf(stderr, "ERR: creating pin dirname %d.\n", i);
		}
		xdp_stats_map_fd[i] = open_bpf_map_file(
			pin_dir, "xdp_stats_map", &xdp_stats_map_info[i]);
		if (xdp_stats_map_fd[i] < 0) {
			fprintf(stderr,
				"ERR: Can not open the XDP stats map file %d.\n",
				i);
			return EXIT_FAIL_BPF;
		}
		err = check_map_fd_info(&xdp_stats_map_info[i],
					&xdp_stats_map_expect);
		if (err) {
			fprintf(stderr,
				"ERR: XDP stats map %d via FD not compatible.\n",
				i);
		}
		printf("Successfully opened the map file of xdp stats map from %s.\n",
		       ifname);
	}

	if (init_power_library()) {
		rte_exit(EXIT_FAILURE, "Failed to init the power library.\n");
	}

	if (init_power_library_on_system()) {
		rte_exit(
			EXIT_FAILURE,
			"Failed to init the power library on the system cores.\n");
	}

	check_lcore_power_caps();
	int lcore_id;
	int ret;
	for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	     lcore_id += CORE_MASK) {
		ret = rte_power_turbo_status(lcore_id);
		if (ret == 1) {
			printf("Turbo boost is enabled on lcore %d.\n",
			       lcore_id);
			disable_turbo_boost(); // Does it for all cores at once
		} else if (ret == 0) {
			printf("Turbo boost is disabled on lcore %d.\n",
			       lcore_id);
		} else {
			RTE_LOG(ERR, POWER,
				"Could not get Turbo Boost status on lcore %d.\n",
				lcore_id);
		}
	}

	// Get some frequency infos about the core
	struct freq_info freq_info = { 0 };
	get_frequency_info(CORE_OFFSET, &freq_info, true);

	printf("Scale frequency of CNF CPU down to maximum.\n");
	for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	     lcore_id += CORE_MASK) {
		ret = rte_power_freq_max(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not scale lcore %d frequency to minimum",
				lcore_id);
		}
	}
	printf("Scale frequency of system CPU up to maximum.\n");
	set_system_pstate(1);

	// Start eBPF map polling and scaling
	printf("Collecting stats from BPF map:\n");
	freq_info.pstate = rte_power_get_freq(CORE_OFFSET);
	freq_info.freq = freq_info.freqs[freq_info.pstate];
	stats_poll(xdp_stats_map_fd, &freq_info);

	/// Save global stats here --> less signaling between single sessions
	/// Get PID with ffpp_power and the simply kill PID
	exit_power_library();
	exit_power_library_on_system();
	printf("\nBye..\n");
	return 0;
}
