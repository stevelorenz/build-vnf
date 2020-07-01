#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include <rte_power.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_timer.h>

#define NUM_CORES 4
#define MAX_PSTATES 32

static int init_power_library(void)
{
	int ret = 0;
	int lcore_id = 0;
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
		ret = rte_power_init(lcore_id);
		if (ret) {
			RTE_LOG(ERR, POWER,
				"Can't init power library on core: %u.\n",
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
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
		struct rte_power_core_capabilities caps;
		ret = rte_power_get_capabilities(lcore_id, &caps);
		if (ret == 0) {
			RTE_LOG(INFO, POWER, "Lcore %d has power capability.\n",
				lcore_id);
			cnt += 1;
		}
	}
	if (cnt == 0) {
		rte_exit(
			EXIT_FAILURE,
			"None of the enabled lcores has the power capability.\n");
	}
}

static void freq_min(void)
{
	int ret;
	int lcore_id;
	printf("Scale frequency down to minimum.\n");
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
		ret = rte_power_freq_min(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not scale lcore %d frequency to minimum",
				lcore_id);
		}
	}
}

static void enable_turbo(void)
{
	int ret;
	int lcore_id;
	printf("Enable Turbo Boost.\n");
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
		ret = rte_power_freq_enable_turbo(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not enable Turbo Boost on lcore %d.\n",
				lcore_id);
		}
	}
}

static void freq_switch(unsigned int us, int num_pstates, bool turbo)
{
	int ret;
	int lcore_id;
	int pstate;
	int max_pstate = 1;
	// Set for-condition according to status of @turbo
	if (turbo) {
		// enable_turbo();
		max_pstate = 0;
	}
	for (pstate = num_pstates - 1; pstate >= max_pstate; pstate--) {
		printf("Scale up to p-state %d.\n", pstate);
		for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
			ret = rte_power_set_freq(lcore_id, pstate);
			if (ret < 0) {
				RTE_LOG(ERR, POWER,
					"Failed to scale frequency for lcore %d.\n",
					lcore_id);
			}
		}
		rte_delay_us_block(us);
	}
	for (pstate = max_pstate; pstate < num_pstates; pstate++) {
		printf("Scale down to p-state %d.\n", pstate);
		for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
			ret = rte_power_set_freq(lcore_id, pstate);
			if (ret < 0) {
				RTE_LOG(ERR, POWER,
					"Failed to scale frequency for lcore %d.\n",
					lcore_id);
			}
		}
		rte_delay_us_block(us);
	}
}

int main(int argc, char *argv[])
{
	// Parse own args first
	char *ptr;
	int c;
	unsigned int us = 1000000; // micro seconds to wait between two scales
	bool turbo = false;
	while ((c = getopt(argc, argv, "s:t")) != -1) {
		switch (c) {
		case 's':
			us = strtoul(optarg, &ptr, 10);
			break;
		case 't':
			turbo = true;
			break;
		case '?':
			if (optopt == 's') {
				fprintf(stderr,
					"Option -%c requires an argument. "
					"Running with default (%d µs) now\n",
					optopt, us);
			} else if (isprint(optopt)) {
				fprintf(stderr, "Unknown option '-%c'.\n",
					optopt);
			} else {
				fprintf(stderr,
					"Unknown option character '\\x%x'.\n",
					optopt);
			}
			// return 0;
			break;
		default:
			return 0;
		}
	}

	// Initialize EAL with remaining args
	int ret;
	argc -= optind + 1; // @1: @getopt already tried to process EAL options
	argv += optind - 1;
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
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
	if (turbo) {
		enable_turbo();
	}

	int num_pstates;
	unsigned int freqs[MAX_PSTATES];
	num_pstates = rte_power_freqs(0, freqs, MAX_PSTATES);

	freq_min();
	printf("Start turbostat now!!!!\n");
	rte_delay_us_block(1000000);
	freq_switch(us, num_pstates, turbo);

	rte_eal_cleanup();
	return 1;
}
