/*
 * About: A cool power manager that manage the CPU frequency based on the
 * statistics provided by the XDP program(s).
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

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
#include "../../../kern/xdp_fwd/common_kern_user.h"

const char *pin_basedir = "/sys/fs/bpf";

static int init_power_library(void)
{
	int ret = 0;
	int lcore_id = 0;
	for (lcore_id = 0; lcore_id < 4; lcore_id++) {
		ret = rte_power_init(lcore_id);
		if (ret) {
			RTE_LOG(ERR, POWER,
				"Can not init power library on core: %u\n",
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
	for (lcore_id = 0; lcore_id < 4; lcore_id++) {
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

static void stats_print(struct stats_record *stats_rec,
			struct stats_record *stats_prev,
			struct measurement *measurement)
{
	struct record *rec, *prev;
	__u64 packets;
	double inter_arrival;
	double period;
	double pps;

	char *fmt = // "%-12s"
		"%d"
		"%'11lld pkts (%'10.0f pps)"
		" \t%'10.8f s"
		" \tperiod:%f\n";
	// const char *action = action2str(2); // @2: xdp_pass

	rec = &stats_rec->stats;
	prev = &stats_prev->stats;

	period = calc_period(rec, prev);
	if (period == 0) {
		return;
	}

	packets = rec->total.rx_packets - prev->total.rx_packets;
	pps = packets / period;

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

	printf(fmt, //action,
	       measurement->count, rec->total.rx_packets, pps, inter_arrival,
	       period);
}

static void stats_collect(int map_fd, struct stats_record *stats_rec)
{
	__u32 key = 0; // Only one entry in our map
	map_collect(map_fd, key, &stats_rec->stats);
}

static void stats_poll(int map_fd, int interval, struct freq_info *freq_info)
{
	struct stats_record prev, record = { 0 };
	struct measurement measurement = { 0 };

	measurement.lcore = rte_lcore_id();
	measurement.min_counts = NUM_READINGS;
	measurement.had_first_packet = false;

	setlocale(LC_NUMERIC, "en_US");
	stats_collect(map_fd, &record);
	usleep(1000000 / 4);

	while (1) {
		prev = record;
		stats_collect(map_fd, &record);
		stats_print(&record, &prev, &measurement);
		get_cpu_utilization(&measurement, freq_info);
		sleep(interval);
	}
}

int main(int argc, char *argv[])
{
	struct bpf_map_info tx_port_map_info = { 0 };
	struct bpf_map_info xdp_stats_map_info = { 0 };
	const struct bpf_map_info tx_port_map_expect = {
		.key_size = sizeof(int),
		.value_size = sizeof(int),
		.max_entries = 256,
	};
	const struct bpf_map_info xdp_stats_map_expect = {
		.key_size = sizeof(__u32),
		.value_size = sizeof(struct datarec),
		.max_entries = 1,
	};

	char pin_dir[PATH_MAX] = "";
	int tx_port_map_fd = 0;
	int xdp_stats_map_fd = 0;
	int interval = 2; // seconds between two consecutive map reads

	// MARK: Currently hard-coded to avoid conflicts with DPDK's CLI parser.
	const char *ifname = "pktgen-out-root";

	int len = 0;
	len = snprintf(pin_dir, PATH_MAX, "%s/%s", pin_basedir, ifname);
	if (len < 0) {
		fprintf(stderr, "ERR: creating pin dirname\n");
	}

	tx_port_map_fd =
		open_bpf_map_file(pin_dir, "tx_port", &tx_port_map_info);
	if (tx_port_map_fd < 0) {
		fprintf(stderr, "ERR: Can not open the tx-port map file.\n");
		return EXIT_FAIL_BPF;
	}

	int err = 0;
	err = check_map_fd_info(&tx_port_map_info, &tx_port_map_expect);
	if (err) {
		fprintf(stderr, "ERR: tx-port map via FD not compatible\n");
		return err;
	}
	printf("Successfully open the map file of tx_port!\n");

	// Open xdp_stats_map from pktgen-out-root interface
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

	// Test if rte_power works.
	int ret = 0;
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Invalid EAL arguements\n");
	}
	argc -= ret;
	argv += ret;

	rte_timer_subsystem_init();

	if (init_power_library()) {
		rte_exit(EXIT_FAILURE, "Failed to init the power library.\n");
	}

	check_lcore_power_caps();
	int lcore_id;
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
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
	lcore_id = 0; // Dummy core for @get_frequency_info
	get_frequency_info(lcore_id, &freq_info, true);

	// Test if scaling works
	// int i = 0;
	// uint32_t freq_index = 0;
	// printf("Try to scale down the frequency of current lcore.\n");
	// for (i = 0; i < 6; ++i) {
	// freq_index = rte_power_get_freq(rte_lcore_id());
	// printf("Current frequency index: %u.\n", freq_index);
	// for (lcore_id = 0; lcore_id < 4; lcore_id++) {
	// // ret = rte_power_freq_up(rte_lcore_id());
	// ret = rte_power_freq_down(lcore_id);
	// if (ret < 0) {
	// RTE_LOG(ERR, POWER,
	// "Failed to scale down the CPU frequency.");
	// }
	// }
	// sleep(1);
	// printf("Current frequency: %f kHz\n",
	//    get_cpu_frequency(lcore_id - 1));
	// }

	printf("Scale frequency down to minimum.\n");
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
		ret = rte_power_freq_min(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not scale lcore %d frequency to minimum",
				lcore_id);
		}
	}

	// Print stats from xdp_stats_map
	printf("Collecting stats from BPF map:\n");
	freq_info.pstate = rte_power_get_freq(0);
	freq_info.freq = freq_info.freqs[freq_info.pstate];
	stats_poll(xdp_stats_map_fd, interval, &freq_info);

	rte_eal_cleanup();
	return 0;
}
