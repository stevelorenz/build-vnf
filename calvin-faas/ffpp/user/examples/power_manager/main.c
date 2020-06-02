/*
 * About: A cool power manager that manage the CPU frequency based on the
 * statistics provided by the XDP program(s).
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_power.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_timer.h>

#include <ffpp/bpf_helpers_user.h>

const char *pin_basedir = "/sys/fs/bpf";

static int init_power_library(void)
{
	int ret = 0;
	int lcore_id = 0;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id)) {
			ret = rte_power_init(lcore_id);
			if (ret) {
				RTE_LOG(ERR, POWER,
					"Can not init power library on core: %u\n",
					lcore_id);
			}
		}
	}
	return ret;
}

static void check_lcore_power_caps(void)
{
	int ret = 0, lcore_id = 0;
	int cnt = 0;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id)) {
			struct rte_power_core_capabilities caps;
			ret = rte_power_get_capabilities(lcore_id, &caps);
			if (ret == 0) {
				RTE_LOG(INFO, POWER,
					"Lcore:%d has power capability.\n",
					lcore_id);
				cnt += 1;
			}
		}
	}
	if (cnt == 0) {
		rte_exit(
			EXIT_FAILURE,
			"None of the enabled lcores has the power capability.");
	}
}

int main(int argc, char *argv[])
{
	struct bpf_map_info tx_port_map_info = { 0 };
	const struct bpf_map_info tx_port_map_expect = {
		.key_size = sizeof(int),
		.value_size = sizeof(int),
		.max_entries = 256,
	};

	char pin_dir[PATH_MAX] = "";
	int tx_port_map_fd = 0;

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
		fprintf(stderr, "ERR: Can not open the map file.\n");
		return EXIT_FAIL_BPF;
	}

	int err = 0;
	err = check_map_fd_info(&tx_port_map_info, &tx_port_map_expect);
	if (err) {
		fprintf(stderr, "ERR: map via FD not compatible\n");
		return err;
	}

	printf("Successfully open the map file of tx_port!\n");

	/* TODO:  <Malte> Open and read traffic monitoring stats from the XDP
	 * program attached at pktgen-out-root interface. */

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
	ret = rte_power_turbo_status(rte_lcore_id());
	if (ret == 1) {
		printf("* Turbo boost is enabled.\n");
	} else {
		printf("* Turbo boost is disabled.\n");
	}

	int i = 0;
	uint32_t freq_index = 0;
	printf("Try to scale down the frequency of current lcore.\n");
	for (i = 0; i < 3; ++i) {
		freq_index = rte_power_get_freq(rte_lcore_id());
		printf("Current frequency index: %u.\n", freq_index);
		ret = rte_power_freq_down(rte_lcore_id());
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Faild to scale down the CPU frequency.");
		}
		sleep(1);
	}

	/* TODO:  <Malte>
	 * Change CPU frequency using DPDK's power management API.
	 * https://doc.dpdk.org/api-20.02/rte__power_8h.html
	 * */

	rte_eal_cleanup();
	return 0;
}
