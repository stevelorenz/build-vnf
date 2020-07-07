/**
 * About: A simple tool to set CPU frequency from userspace.
 * Input args (only the first argument will be processed)
 * @arg -l: Scale CPU frequency down to its minimum
 * @arg -h: Scale CPU frequency up to its maximum
 * @arg -t: Enable Turbo Boost
 * @arg -f [freq]: Set CPU frquency to [freq] (in KHz)
 * @arg -p [pstate]: Send CPU to [pstate]
 *
 * ToDo: Add interactive mode
 */

#include <stdio.h>
#include <unistd.h>

#include <rte_power.h>

#include <ffpp/bpf_helpers_user.h>

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

static void freq_max(void)
{
	int ret;
	int lcore_id;
	printf("Scale frequency up to maximum.\n");
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
		ret = rte_power_freq_max(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not scale lcore %d frequency to maximum.\n",
				lcore_id);
		}
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

static void freq_turbo(void)
{
	int ret;
	int lcore_id;
	printf("Enable Turbo Boost.\n");
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
		ret = rte_power_freq_enable_turbo(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not enable Turbo Boost on lcore %d",
				lcore_id);
		}
		ret = rte_power_freq_max(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not boost frequency on lcore %d",
				lcore_id);
		}
	}
}

static void set_freq(int pstate)
{
	int ret;
	int lcore_id;
	for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
		ret = rte_power_set_freq(lcore_id, pstate);
		if (ret < 0) {
			RTE_LOG(ERR, POWER, "Failed to scale CPU frequency.\n");
		}
	}
}

static void check_freq(unsigned int freq, unsigned int *freqs, int num_freqs)
{
	if (freq >= freqs[0]) {
		printf("Given frequency too high, try to enter Turbo Boost.\n");
		freq_turbo();
	} else if (freq >= freqs[num_freqs - 1]) {
		int freq_idx;
		for (freq_idx = num_freqs - 1; freq_idx > 0; freq_idx--) {
			if (freqs[freq_idx] >= freq) {
				set_freq(freq_idx);
				break;
			}
		}
	} else {
		printf("Given frequency too low, scale down to min frequency.\n");
		freq_min();
	}
}

static void check_pstate(int pstate, int num_pstates)
{
	if (pstate == 0) {
		printf("Given frequency too high, try to enter Turbo Boost.\n");
		freq_turbo();
	} else if (pstate < num_pstates) {
		set_freq(pstate);
	} else {
		printf("Given frequency too low, scale down to min frequency.\n");
		freq_min();
	}
}

int main(int argc, char *argv[])
{
	gettime();
	if (argc == 1) {
		printf("No option was given, terminate programm.\n");
		printf("Please run with one of the folling options: \n"
		       "-l: to scale frequency to minimum\n"
		       "-h: to sclae frequency to maximum\n"
		       "-t: to enable Turbo Boost\n"
		       "-f [freq]: to scale frequency to [freq] KHz\n"
		       "-p [pstate]: to enter p-state [pstate]\n\n"
		       "Thank you and goodbye!\n");
		return 0;
	}

	if (init_power_library()) {
		rte_exit(EXIT_FAILURE, "Failed to init the power library.\n");
	}

	check_lcore_power_caps();
	int ret;
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

	int num_freqs;
	unsigned int freqs[MAX_PSTATES];
	num_freqs = rte_power_freqs(0, freqs, MAX_PSTATES);

	char *ptr; // dummy pointer for function @strtoul
	unsigned int freq;
	int pstate;
	int c;
	int i;
	// @optind < 2: process only the first argument
	// Lets hope the compiler does not change the condition order ;)
	while ((optind < 2) && ((c = getopt(argc, argv, "f:hlp:t")) != -1)) {
		// Or just set a flag here
		switch (c) {
		case 'f':
			freq = strtoul(optarg, &ptr, 10);
			check_freq(freq, freqs, num_freqs);
			break;
		case 'h':
			freq_max();
			break;
		case 'l':
			freq_min();
			break;
		case 'p':
			pstate = strtoul(optarg, &ptr, 10);
			check_pstate(pstate, num_freqs);
			break;
		case 't':
			freq_turbo();
			break;
		case '?':
			if (optopt == 'f' || optopt == 'p') {
				fprintf(stderr,
					"Option -%c requires an argument.\n",
					optopt);
			} else if (isprint(optopt)) {
				fprintf(stderr, "Unknown option '-%c'.\n",
					optopt);
			} else {
				fprintf(stderr,
					"Unknown option character '\\x%x'.\n",
					optopt);
			}
			return 0;
		default:
			return 0;
		}
	}

	for (i = optind; i < argc; i++) {
		printf("Non-option or not processed argument %s\n", argv[i]);
		// return 0;
	}

	pstate = rte_power_get_freq(0);
	printf("Scaled frequency to %d KHz (p-state %d).\n", freqs[pstate],
	       pstate);

	return 1;
}
