#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <ffpp/scaling_helpers_user.h>
#include <ffpp/bpf_helpers_user.h> // NANOSEC_PER_SEC

void get_frequency_info(int lcore, struct freq_info *f, bool debug)
{
	f->num_freqs = rte_power_freqs(lcore, f->freqs, MAX_PSTATES);
	f->pstate = rte_power_get_freq(lcore);
	f->freq = f->freqs[f->pstate];

	if (debug) {
		printf("Number of possible p-states %d.\n", f->num_freqs);
		for (int i = 0; i < f->num_freqs; i++) {
			printf("Frequency: %d KHz; Index %d\n", f->freqs[i], i);
		}
		printf("Current p-staste %d and frequency: %d KHz.\n",
		       f->pstate, f->freq);
	}
}

double get_cpu_frequency(int lcore)
{
	FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
	if (cpuinfo == NULL) {
		fprintf(stderr, "ERR: Couldn't get CPU frequency");
		return EXIT_FAILURE;
	}

	const char *key = "MHz";
	char str[255];
	double freq = 0.0;
	int cnt = 0;

	while ((fgets(str, sizeof str, cpuinfo)) != NULL) {
		if (strstr(str, key) != NULL) {
			if (cnt == lcore) {
				char *s = strtok(str, ":");
				s = strtok(NULL,
					   s); // get second element (frequency)
				freq = atof(s);
				break;
			} else {
				cnt++;
			}
		}
	}
	fclose(cpuinfo);

	return freq * 1e3;
}

void set_turbo()
{
	int ret;
	int lcore_id;
	/// Enable already in the begining?
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

void set_pstate(struct freq_info *f, int pstate)
{
	if (pstate == 0) {
		/// Do we want to use Turbo Boost?
		set_turbo();
		f->pstate = pstate;
		f->freq = f->freqs[pstate]; // Actual frequency will be higher
	} else {
		int ret;
		int lcore_id;
		for (lcore_id = 0; lcore_id < NUM_CORES; lcore_id++) {
			ret = rte_power_set_freq(lcore_id, pstate);
			if (ret < 0) {
				RTE_LOG(ERR, POWER,
					"Failed to scale frequency of lcore %d.\n",
					lcore_id);
			}
		}
		f->pstate = pstate;
		f->freq = f->freqs[pstate];
	}
}

int calc_pstate(struct measurement *m, struct freq_info *f)
{
	// First, calc which frequency is needed for a good CPU utilization
	unsigned int new_freq;
	// new_freq = (C_PACKET / (m->inter_arrival_time * UTIL_THRESHOLD_UP);
	new_freq = (unsigned int)CPU_FREQ(m->inter_arrival_time);

	// Now, choose the next highest frequency
	if (new_freq >= f->freqs[0]) {
		return 0; // p-state for Turbo Boost
	} else {
		int freq_idx;
		for (freq_idx = f->num_freqs - 1; freq_idx > 0; freq_idx--) {
			if (f->freqs[freq_idx] >= new_freq) {
				return freq_idx;
			}
		}
	}
}

void check_frequency_scaling(struct measurement *m, struct freq_info *f)
{
	if (m->avg_cpu_util < UTIL_THRESHOLD_DOWN) {
		/// Increment policy according to deviation from threshold?
		/// Check DPDK solution
		m->scale_down_count += 1;
		m->scale_up_count = 0;
		if (m->scale_down_count > COUNTER_THRESHOLD) {
			int pstate;
			pstate = calc_pstate(m, f);
			if (pstate != f->pstate) {
				set_pstate(f, pstate);
			}
			m->scale_down_count = 0;
		}
	} else if (m->avg_cpu_util > UTIL_THRESHOLD_UP) {
		m->scale_up_count += 1;
		m->scale_down_count = 0;
		if (m->scale_up_count > COUNTER_THRESHOLD) {
			int pstate;
			pstate = calc_pstate(m, f);
			if (pstate != f->pstate) {
				set_pstate(f, pstate);
			}
			m->scale_up_count = 0;
		}
	}
}

void get_cpu_utilization(struct measurement *m, struct freq_info *f)
{
	int i;
	double sum = 0.0;
	// double cpu_freq = get_cpu_frequency(m->lcore);
	m->cpu_util[m->idx] = CPU_UTIL(m->inter_arrival_time, f->freq);
	/// Is the moving average beneficial or are counters sufficent
	/// The inter-arrival time is aready an average
	// set @NUM_READINGS to 1 for tests, don't remove directly ;)
	if (m->count >= NUM_READINGS) {
		for (i = 0; i < NUM_READINGS; i++) {
			sum += m->cpu_util[i];
		}
		m->avg_cpu_util = sum / NUM_READINGS;
		check_frequency_scaling(m, f);
	}
	printf("CPU frequency %f Hz\n", get_cpu_frequency(m->lcore));
	printf("Current CPU utilization: %f\nAverage CPU utilization: %f\n\n",
	       m->cpu_util[m->idx], m->avg_cpu_util);
}

double calc_period(struct record *r, struct record *p)
{
	double period_ = 0;
	__u64 period = 0;

	period = r->timestamp - p->timestamp;
	if (period > 0) {
		period_ = ((double)period / NANOSEC_PER_SEC);
	}

	return period_;
}
