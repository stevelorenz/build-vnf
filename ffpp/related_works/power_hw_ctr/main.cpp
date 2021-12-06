/*
 * Hardware Counter (HC) based utilization estimation and power management.
 *
 * Reference paper(s):
 *
 * - A Black-Box Approach for Estimating Utilization of Polled IO Network Functions.
 */

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/sysinfo.h>
#include <sys/types.h>

#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_power.h>
#include <rte_spinlock.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#define FREQ_WINDOW_SIZE 32
#define RTE_LOGTYPE_POWER_MANAGER RTE_LOGTYPE_USER1

#define POLL_INTERVAL 1e6
#define PRINT_LOOP_COUNT (1000000 / POLL_INTERVAL)
#define IA32_PERFEVTSEL0 0x186
#define IA32_PERFEVTSEL1 0x187
#define IA32_PERFCTR0 0xc1
#define IA32_PERFCTR1 0xc2
#define IA32_PERFEVT_BRANCH_HITS 0x05300c4
#define IA32_PERFEVT_BRANCH_MISS 0x05300c5

/* TODO:  <19-09-20, Zuo> Add two events used by the references paper.*/

static volatile bool force_quit = false;
static uint64_t g_branches, g_branch_misses;
static bool g_pm_on = true;
static uint32_t test_rounds = 3;

enum { FREQ_UNKNOWN, FREQ_MIN, FREQ_MAX };

struct core_details {
	uint32_t lcore_id;
	uint64_t last_branches;
	uint64_t last_branch_misses;
	uint16_t oob_enabled;
	int msr_fd;
	uint16_t freq_directions[FREQ_WINDOW_SIZE];
	uint16_t freq_window_idx;
	uint16_t freq_state;
};

struct core_info {
	uint16_t core_count;
	struct core_details *cd;
	float branch_ratio_threshold;
};

struct freq_info {
	rte_spinlock_t power_sl;
	uint32_t freqs[RTE_MAX_LCORE_FREQS];
	uint32_t num_freqs;
} __rte_cache_aligned;

static struct freq_info global_core_freq_info[RTE_MAX_LCORE];

static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
		       signum);
		force_quit = true;
	}
}

static int core_info_init(struct core_info *ci, float branch_ratio_threshold)
{
	uint16_t i = 0;
	uint32_t lcore_id = 0;
	ci->core_count = rte_lcore_count();

	ci->branch_ratio_threshold = branch_ratio_threshold;
	ci->cd = static_cast<core_details *>(
		malloc(ci->core_count * sizeof(struct core_details)));
	memset(ci->cd, 0, ci->core_count * sizeof(struct core_details));
	if (!ci->cd) {
		RTE_LOG(ERR, EAL, "Failed to allocate memory for core info.\n");
		return -1;
	}
	RTE_LCORE_FOREACH(lcore_id)
	{
		ci->cd[i].lcore_id = lcore_id;
		i++;
	}

	printf("* %d lcores is enabled: \n", ci->core_count);
	for (i = 0; i < ci->core_count; ++i) {
		printf("- Lcore ID: %u\n", ci->cd[i].lcore_id);
	}

	return 0;
}

static int add_core_to_monitor(struct core_info *ci, uint32_t idx)
{
	RTE_LOG(INFO, EAL, "Add lcore: %u to the monitor.\n",
		ci->cd[idx].lcore_id);
	char proc_file[PATH_MAX];
	int ret;
	snprintf(proc_file, PATH_MAX, "/dev/cpu/%d/msr", ci->cd[idx].lcore_id);
	ci->cd[idx].msr_fd = open(proc_file, O_RDWR | O_SYNC);
	if (ci->cd[idx].msr_fd < 0) {
		RTE_LOG(ERR, EAL, "Failed to open MSR file for lcore.");
	}

	// Setup branch counters
	long setup;
	setup = IA32_PERFEVT_BRANCH_HITS;
	ret = pwrite(ci->cd[idx].msr_fd, &setup, sizeof(setup),
		     IA32_PERFEVTSEL0);
	if (ret < 0) {
		RTE_LOG(ERR, EAL,
			"Unable to set branch hits counter for lcore");
		return -1;
	}
	setup = IA32_PERFEVT_BRANCH_MISS;
	ret = pwrite(ci->cd[idx].msr_fd, &setup, sizeof(setup),
		     IA32_PERFEVTSEL1);
	if (ret < 0) {
		RTE_LOG(ERR, EAL,
			"Unable to set branch miss counter for lcore");
		return -1;
	}

	// Re-open the file as read-only to not hog the resource
	close(ci->cd[idx].msr_fd);
	ci->cd[idx].msr_fd = open(proc_file, O_RDONLY);
	ci->cd[idx].oob_enabled = 1;

	return 0;
}

static int remove_core_from_monitor(struct core_info *ci, uint32_t idx)
{
	// Re-open the file to disable counters.
	if (ci->cd[idx].msr_fd != 0) {
		close(ci->cd[idx].msr_fd);
	}

	char proc_file[PATH_MAX];
	long setup;
	int ret;
	snprintf(proc_file, PATH_MAX, "/dev/cpu/%d/msr", ci->cd[idx].lcore_id);
	ci->cd[idx].msr_fd = open(proc_file, O_RDWR | O_SYNC);
	if (ci->cd[idx].msr_fd < 0) {
		RTE_LOG(ERR, EAL,
			"Error opening MSR file for core %d "
			"(is msr kernel module loaded?)\n",
			idx);
		return -1;
	}
	setup = 0x0; /* clear event */
	ret = pwrite(ci->cd[idx].msr_fd, &setup, sizeof(setup),
		     IA32_PERFEVTSEL0);
	if (ret < 0) {
		RTE_LOG(ERR, EAL, "unable to set counter for core %u\n", idx);
		return ret;
	}
	setup = 0x0; /* clear event */
	ret = pwrite(ci->cd[idx].msr_fd, &setup, sizeof(setup),
		     IA32_PERFEVTSEL1);
	if (ret < 0) {
		RTE_LOG(ERR, EAL, "unable to set counter for core %u\n", idx);
		return ret;
	}

	close(ci->cd[idx].msr_fd);
	ci->cd[idx].msr_fd = 0;
	ci->cd[idx].oob_enabled = 0;
	return 0;
}

static int power_manager_init(struct core_info *ci)
{
	uint32_t max_core_num = 0;
	uint32_t num_freqs = 0;
	uint32_t i = 0;
	int ret = 0;

	rte_power_set_env(PM_ENV_NOT_SET);
	if (ci->core_count > RTE_MAX_LCORE) {
		max_core_num = RTE_MAX_LCORE;
	} else {
		max_core_num = ci->core_count;
	}
	for (i = 0; i < max_core_num; ++i) {
		if (rte_power_init(ci->cd[i].lcore_id) < 0) {
			RTE_LOG(ERR, EAL,
				"Unable to init power manager for lcore: %u\n",
				ci->cd[i].lcore_id);
		}
		num_freqs = rte_power_freqs(i, global_core_freq_info[i].freqs,
					    RTE_MAX_LCORE_FREQS);
		if (num_freqs == 0) {
			RTE_LOG(ERR, EAL,
				"Unable to get freq list of lcore: %u\n",
				ci->cd[i].lcore_id);
		}
		global_core_freq_info[i].num_freqs = num_freqs;
		rte_spinlock_init(&global_core_freq_info[i].power_sl);
		ret = add_core_to_monitor(ci, i);
		if (ret < 0) {
			RTE_LOG(ERR, EAL,
				"Failed to enable monitoring on the core.\n");
			return -1;
		}
	}

	return 0;
}

static int power_manager_exit(struct core_info *ci)
{
	uint32_t max_core_num;
	uint32_t i;

	if (ci->core_count > RTE_MAX_LCORE) {
		max_core_num = RTE_MAX_LCORE;
	} else {
		max_core_num = ci->core_count;
	}
	for (i = 0; i < max_core_num; ++i) {
		if (rte_power_exit(ci->cd[i].lcore_id) < 0) {
			RTE_LOG(ERR, EAL,
				"Unable to shutdown power manager for lcore: %u\n",
				ci->cd[i].lcore_id);
			return -1;
		}
		remove_core_from_monitor(ci, i);
	}

	return 0;
}

static void scale_all_lcores_max(struct core_info *ci)
{
	uint32_t i = 0;
	for (i = 0; i < ci->core_count; ++i) {
		RTE_LOG(DEBUG, EAL, "[Scale loop] Scale lcore: %u to max!\n",
			ci->cd[i].lcore_id);
		rte_spinlock_lock(&global_core_freq_info[i].power_sl);
		rte_power_freq_max(ci->cd[i].lcore_id);
		rte_spinlock_unlock(&global_core_freq_info[i].power_sl);
	}
}

static void scale_all_lcores_min(struct core_info *ci)
{
	uint32_t i = 0;
	for (i = 0; i < ci->core_count; ++i) {
		RTE_LOG(DEBUG, EAL, "[Scale loop] Scale lcore: %u to min!\n",
			ci->cd[i].lcore_id);
		rte_spinlock_lock(&global_core_freq_info[i].power_sl);
		rte_power_freq_min(ci->cd[i].lcore_id);
		rte_spinlock_unlock(&global_core_freq_info[i].power_sl);
	}
}

static float apply_policy(struct core_info *ci, uint32_t idx)
{
	float ratio;
	int ret;
	int64_t hits_diff, miss_diff;
	uint64_t branches, branch_misses;
	uint64_t counter = 0;
	uint64_t last_branches = 0, last_branch_misses = 0;
	int freq_window_idx, up_count = 0, i;

	last_branches = ci->cd[idx].last_branches;
	last_branch_misses = ci->cd[idx].last_branch_misses;

	ret = pread(ci->cd[idx].msr_fd, &counter, sizeof(counter),
		    IA32_PERFCTR0);
	if (ret < 0) {
		RTE_LOG(ERR, EAL, "Failed to read the counter for lcore: %u\n",
			ci->cd[idx].lcore_id);
	}
	branches = counter;

	counter = 0;
	ret = pread(ci->cd[idx].msr_fd, &counter, sizeof(counter),
		    IA32_PERFCTR1);
	if (ret < 0) {
		RTE_LOG(ERR, EAL, "Failed to read the counter for lcore: %u\n",
			ci->cd[idx].lcore_id);
	}
	branch_misses = counter;

	ci->cd[idx].last_branches = branches;
	ci->cd[idx].last_branch_misses = branch_misses;

	branches >>= 1;
	last_branches >>= 1;
	hits_diff = (int64_t)(branches) - (int64_t)(last_branches);
	RTE_LOG(DEBUG, EAL, "branches: %lu, branch_misses: %lu\n", branches,
		branch_misses);
	// Counter is probably overflow.
	if (hits_diff < 0) {
		return -1.0;
	}

	branch_misses >>= 1;
	last_branch_misses >>= 1;
	miss_diff = (int64_t)branch_misses - (int64_t)last_branch_misses;
	if (miss_diff <= 0) {
		/* Likely a counter overflow condition, skip this round */
		return -1.0;
	}

	g_branches = hits_diff;
	g_branch_misses = miss_diff;
	if (hits_diff < (POLL_INTERVAL * 100)) {
		RTE_LOG(DEBUG, EAL,
			"No workload detected on lcore %u Scale the frequency to MIN.\n",
			ci->cd[idx].lcore_id);
		if (likely(g_pm_on)) {
			scale_all_lcores_min(ci);
		}
		ci->cd[idx].freq_state = FREQ_MIN;
		return -1.0;
	}

	ratio = (float)miss_diff * (float)100 / (float)hits_diff;

	// Perform CPU frequency scaling mechanism to save power.

	/*
	 * Store the last few directions that the ratio indicates
	 * we should take. If there's on 'up', then we scale up
	 * quickly. If all indicate 'down', only then do we scale
	 * down. Each core_details struct has it's own array.
	 */
	freq_window_idx = ci->cd[idx].freq_window_idx;
	if (ratio > ci->branch_ratio_threshold) {
		ci->cd[idx].freq_directions[freq_window_idx] = 1;
	} else {
		ci->cd[idx].freq_directions[freq_window_idx] = 0;
	}
	freq_window_idx++;
	freq_window_idx = freq_window_idx & (FREQ_WINDOW_SIZE - 1);
	ci->cd[idx].freq_window_idx = freq_window_idx;
	up_count = 0;
	for (i = 0; i < FREQ_WINDOW_SIZE; ++i) {
		up_count += ci->cd[idx].freq_directions[i];
	}

	if (up_count == 0) {
		if (ci->cd[idx].freq_state != FREQ_MIN) {
			RTE_LOG(DEBUG, EAL,
				"Scale up the frequency to the MIN!\n");
			scale_all_lcores_min(ci);
			ci->cd[idx].freq_state = FREQ_MIN;
		}
	} else {
		if (ci->cd[idx].freq_state != FREQ_MAX) {
			RTE_LOG(DEBUG, EAL,
				"Scale up the frequency to the MAX!\n");
			if (likely(g_pm_on)) {
				scale_all_lcores_max(ci);
			}
			ci->cd[idx].freq_state = FREQ_MAX;
		}
	}

	return ratio;
}

static void run_branch_monitor(struct core_info *ci)
{
	using namespace std;
	int print = 0;
	int printed = 0;
	float ratio = 0.0;
	int reads = 0;

	uint16_t r = 0;
	vector<double> monitor_latencies;

	fstream csv_file;
	string csv_file_name =
		"./monitor_latencies_" + to_string(ci->core_count) + ".csv";
	csv_file.open(csv_file_name, fstream::out | fstream::app);
	cout << fixed << setprecision(17) << endl;
	csv_file << fixed << setprecision(17) << endl;

	uint64_t start_tsc, diff_tsc;
	while (!force_quit) {
		r += 1;
		int j;
		print++;
		printed = 0;

		start_tsc = rte_get_tsc_cycles();

		for (j = 0; j < ci->core_count; ++j) {
			ratio = apply_policy(ci, j);
			if ((print > PRINT_LOOP_COUNT)) {
				printf("lcore %u: %.4f {%lu} {%d}",
				       ci->cd[j].lcore_id, ratio, g_branches,
				       reads);
				printed = 1;
				reads = 0;
			} else {
				reads++;
			}
		}
		diff_tsc = rte_get_tsc_cycles() - start_tsc;
		monitor_latencies.push_back(double(diff_tsc) /
					    rte_get_tsc_hz());
		if (print > PRINT_LOOP_COUNT) {
			if (printed) {
				printf("\n");
			}
			print = 0;
		}
		if (r == test_rounds) {
			force_quit = true;
		}
	}
	cout << "Monitor latencies (microseconds):" << endl;
	for (auto t : monitor_latencies) {
		t = t * 1e6; // microseconds.
		cout << t << ",";
		csv_file << t << ",";
	}
	cout << endl;
	csv_file.close();
}

int main(int argc, char *argv[])
{
	// Init EAL environment
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	argc -= ret;
	argv += ret;

	int opt = 0;
	float branch_ratio_threshold = 0.1;
	while ((opt = getopt(argc, argv, "b:r:n")) != -1) {
		switch (opt) {
		case 'b':
			if (strlen(optarg)) {
				branch_ratio_threshold = atof(optarg);
				if (branch_ratio_threshold <= 0.0) {
					rte_eal_cleanup();
					return -1;
				}
			}
			break;
		case 'n':
			g_pm_on = false;
			break;
		case 'r':
			test_rounds = atoi(optarg);
			break;

		default:
			rte_eal_cleanup();
			return 0;
		}
	}

	printf("* The threshold of the branch ratio is: %f\n",
	       branch_ratio_threshold);
	printf(" The power management is %s.\n",
	       g_pm_on == true ? "enabled" : "disabled");
	printf(" Test rounds: %u\n", test_rounds);

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	struct core_info ci;
	ret = core_info_init(&ci, branch_ratio_threshold);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Failed to init core info.\n");
	}
	ret = power_manager_init(&ci);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Failed to init power manager.\n");
	}

	RTE_LOG(INFO, EAL, "Run core monitor on the main core :%u\n",
		rte_get_main_lcore());
	run_branch_monitor(&ci);

	rte_eal_mp_wait_lcore();

	RTE_LOG(INFO, EAL, "* Run the cleanups\n");
	ret = power_manager_exit(&ci);
	if (ret < 0) {
		RTE_LOG(ERR, EAL, "Failed to exit power manager.\n");
	}
	free(ci.cd);
	rte_eal_cleanup();
	return 0;
}
