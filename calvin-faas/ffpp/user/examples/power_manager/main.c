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
#include "../../../kern/xdp_fwd/common_kern_user.h"

// #define RELEASE 1
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
int g_csv_empty_cnt_threshold =
	(3 * 1e6) / IDLE_INTERVAL; //50; // Calc depending on interval
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

static void collect_global_stats(struct freq_info *f, struct measurement *m,
				 struct scaling_info *si)
{
	// if (m->had_first_packet) {
	g_csv_ts[g_csv_num_val] = get_time_of_day();
	g_csv_pps[g_csv_num_val] = (1 / m->inter_arrival_time); //ts->pps;
	g_csv_iat[g_csv_num_val] = m->inter_arrival_time;
	g_csv_freq[g_csv_num_val] = f->freq;
	g_csv_num_val++;
	if (m->empty_cnt == 0) { //(ts[0].delta_packets > 0) {
		g_csv_empty_cnt = 0;
	} else {
		if (!g_csv_saved_stream) {
			g_csv_pps[g_csv_num_val - 1] = 0.0;
			g_csv_cpu_util[g_csv_num_val - 1] = 0.0;
			g_csv_empty_cnt += 1;
		}
		if (g_csv_empty_cnt > g_csv_empty_cnt_threshold) {
			write_csv_file();
			// write_csv_file_fb_out();
			g_csv_saved_stream = true;
			g_csv_empty_cnt = 0;
			g_csv_num_val = 0;
			g_csv_num_round++;
			si->scaled_to_min = false;
			m->had_first_packet = false;
		}
	}
	// }
}

static void stats_print(struct stats_record *stats_rec,
			struct stats_record *stats_prev, struct measurement *m,
			struct scaling_info *si)
{
	struct record *rec, *prev;
	struct traffic_stats t_s = { 0 };

	rec = &stats_rec->stats;
	prev = &stats_prev->stats;

	calc_traffic_stats(m, rec, prev, &t_s, si);

	printf("%ld %11lld pkts (%10.0f pps) \t%10.8f s \tperiod:%f\n",
	       m->cnt, rec->total.rx_packets, t_s.pps, m->inter_arrival_time,
	       t_s.period);
}

static void stats_collect(int map_fd, struct stats_record *stats_rec)
{
	__u32 key = 0; // Only one entry in our map
	map_collect(map_fd, key, &stats_rec->stats);
}

static void stats_poll(int map_fd, struct freq_info *freq_info)
{
	struct stats_record prev, record = { 0 };
	struct measurement m = { 0 };
	struct scaling_info si = { 0 };
	struct last_stream_settings lss = { 0 };

	m.lcore = rte_lcore_id();
	m.min_cnts = NUM_READINGS_SMA;
	m.had_first_packet = false;
	si.up_trend = false; /// Do we have to set them to false explicitly?
	si.down_trend = false;
	si.scale_to_min = false;
	si.need_scale = false;
	si.scaled_to_min = false;
	si.restore_settings = false;

	setlocale(LC_NUMERIC, "en_US");
	stats_collect(map_fd, &record);
	usleep(1000000 / 4);

	while (!force_quit) {
		prev = record;
		stats_collect(map_fd, &record);
		stats_print(&record, &prev, &m, &si);
		if (si.restore_settings) {
			restore_last_stream_settings(&lss, freq_info, &si);
			m.valid_vals = -1; // Skip first burst
			// get_cpu_utilization(&m, freq_info);
			// g_csv_cpu_util[g_csv_num_val - 1] = m.cpu_util[m.idx];
		}
		if (m.had_first_packet) {
			collect_global_stats(freq_info, &m, &si);
			// g_csv_freq[g_csv_num_val - 1] = freq_info->freq;
		}
		// if (m.cnt > 0 && m.had_first_packet) {
		if (m.valid_vals > 0) {
			get_cpu_utilization(&m, freq_info);
			g_csv_cpu_util[g_csv_num_val - 1] = m.cpu_util[m.idx];
			// g_csv_freq[g_csv_num_val - 1] = freq_info->freq;
			// }
			// if (m.valid_vals > 1) {
			calc_sma(&m);
			calc_wma(&m);
			check_traffic_trends(&m, &si);
			check_frequency_scaling(&m, freq_info, &si);
			/// Move scaling to beginning => nicer plots ;)
			/// Rename scale_to_min with set_c1 etc.
			if (si.scale_to_min) {
				// Store settings of last stream
				lss.last_pstate = 1; //freq_info->pstate;
				lss.last_sma = m.sma_cpu_util;
				lss.last_sma_std_err = m.sma_std_err;
				lss.last_wma = m.wma_cpu_util;
				/// Scale down before sleep? -> c1e
				/// else just c1
				if (freq_info->pstate !=
				    (freq_info->num_freqs - 1)) {
					printf("Scale due to empty polls\n");
					si.next_pstate =
						freq_info->num_freqs - 1,
					set_pstate(freq_info, &si);
				} else {
					printf("Already at min\n");
				}
				// set_c1("off");
				// Reset flags
				si.scale_to_min = false;
				si.scaled_to_min = true;
				si.up_trend = false;
				si.down_trend = false;
				si.need_scale = false;
				m.valid_vals = 0;
				//m.had_first_packet = false;
			}
			/// Check for empty_cnt and wait with scaling if !=0?
			if (si.need_scale) {
				calc_pstate(&m, freq_info, &si);
				/// Check timestamp or so if we're allowed to scale
				if (si.next_pstate != freq_info->pstate) {
					set_pstate(freq_info, &si);
					m.valid_vals = 0;
				} else {
					si.need_scale = false;
				}
			}
			usleep(INTERVAL);
		} else {
			usleep(IDLE_INTERVAL);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[])
{
	if (argc < 1) {
		fprintf(stderr, "Please supply ingress interface name");
		return -1;
	}
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

	if (init_power_library()) {
		rte_exit(EXIT_FAILURE, "Failed to init the power library.\n");
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

	// Test if scaling works
	// int i = 0;
	// uint32_t freq_index = 0;
	// printf("Try to scale down the frequency of current lcore.\n");
	// for (i = 0; i < 6; ++i) {
	// for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	//  lcore_id += CORE_MASK) {
	// // ret = rte_power_freq_up(rte_lcore_id());
	// ret = rte_power_freq_down(lcore_id);
	// if (ret < 0) {
	// RTE_LOG(ERR, POWER,
	// "Failed to scale down the CPU frequency.");
	// }
	// }
	// sleep(6);
	// freq_index = rte_power_get_freq(CORE_OFFSET);
	// printf("Current frequency index: %u.\n", freq_index);
	// printf("Current frequency: %d kHz\n",
	//    freq_info.freqs[freq_index]);
	// }

	printf("Scale frequency down to minimum.\n");
	for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	     lcore_id += CORE_MASK) {
		ret = rte_power_freq_min(lcore_id);
		// ret = rte_power_freq_max(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not scale lcore %d frequency to minimum",
				lcore_id);
		}
	}

	// Print stats from xdp_stats_map
	printf("Collecting stats from BPF map:\n");
	freq_info.pstate = rte_power_get_freq(CORE_OFFSET);
	freq_info.freq = freq_info.freqs[freq_info.pstate];
	stats_poll(xdp_stats_map_fd, &freq_info);

	/// Save global stats here --> less signaling between single sessions
	/// Get PID with ffpp_power and the simply kill PID
	exit_power_library();
	printf("\nBye..\n");
	return 0;
}

// --- Load of BPF tx_port map and ini of EAL ---
// - Currently not needed
// struct bpf_map_info tx_port_map_info = { 0 };
// const struct bpf_map_info tx_port_map_expect = {
// .key_size = sizeof(int),
// .value_size = sizeof(int),
// .max_entries = 256,
// };
// int tx_port_map_fd = 0;

// tx_port_map_fd =
// open_bpf_map_file(pin_dir, "tx_port", &tx_port_map_info);
// if (tx_port_map_fd < 0) {
// fprintf(stderr, "ERR: Can not open the tx-port map file.\n");
// return EXIT_FAIL_BPF;
// }

// int err = 0;
// err = check_map_fd_info(&tx_port_map_info, &tx_port_map_expect);
// if (err) {
// fprintf(stderr, "ERR: tx-port map via FD not compatible\n");
// return err;
// }
// printf("Successfully open the map file of tx_port!\n");

// // Test if rte_power works.
// int ret = 0;
// ret = rte_eal_init(argc, argv);
// if (ret < 0) {
// rte_exit(EXIT_FAILURE, "Invalid EAL arguements\n");
// }
// argc -= ret;
// argv += ret;

// rte_timer_subsystem_init();

// rte_eal_cleanup();
