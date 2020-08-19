#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>

#include <zmq.h>
#include <jansson.h>

#include <ffpp/scaling_helpers_user.h>
#include <ffpp/bpf_helpers_user.h> // NANOSEC_PER_SEC
#include <ffpp/general_helpers_user.h>

#ifdef RELEASE
#define printf(fmt, ...) (0)
#endif

void get_frequency_info(int lcore, struct freq_info *f, bool debug)
{
	f->num_freqs = rte_power_freqs(lcore, f->freqs, MAX_PSTATES);
	f->pstate = rte_power_get_freq(lcore);
	f->freq = f->freqs[f->pstate];

	if (debug) {
		printf("Number of possible p-states %d.\n", f->num_freqs);
		for (unsigned int i = 0; i < f->num_freqs; i++) {
			printf("Frequency: %d kHz; Index %d\n", f->freqs[i], i);
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
	for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	     lcore_id += CORE_MASK) {
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

void disable_turbo_boost()
{
	int ret;
	int lcore_id;
	for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
	     lcore_id += CORE_MASK) {
		ret = rte_power_freq_disable_turbo(lcore_id);
		if (ret < 0) {
			RTE_LOG(ERR, POWER,
				"Could not disable Turbo Boost on lcore %d",
				lcore_id);
		}
	}
}

void set_c1(char *msg)
{
	int ret;
	json_t *root = json_object();
	char *req_msg;

	ret = json_object_set_new(root, "action", json_string(msg));
	if (ret < 0) {
		RTE_LOG(ERR, POWER, "Could not set action for c1");
	}
	req_msg = json_dumps(root, 0);
	printf("Request message: %s\n", req_msg);

	void *context = zmq_ctx_new();
	void *req = zmq_socket(context, ZMQ_REQ);
	char buf[10];

	zmq_connect(req, "ipc:///tmp/ffpp.sock");
	zmq_send(req, req_msg, strlen(req_msg), 0);
	zmq_recv(req, buf, 10, 0);
	printf("Response message: %s\n", buf);

	zmq_close(req);
	zmq_ctx_destroy(context);
	free(req_msg);
	json_decref(root);
}

void set_pstate(struct freq_info *f, struct scaling_info *si)
{
	// Do not use turbo boost
	if (si->next_pstate == 0) {
		si->next_pstate = 1;
		/// Do we want to use Turbo Boost?
		// set_turbo();
		// f->pstate = si->next_pstate;
		// f->freq =
		// f->freqs[f->pstate]; // Actual frequency will be higher
	} else if (si->next_pstate >= f->num_freqs) {
		si->next_pstate = f->num_freqs - 1;
	}
	if (si->next_pstate != rte_power_get_freq(CORE_OFFSET)) {
		int ret;
		int lcore_id;
		for (lcore_id = CORE_OFFSET; lcore_id < NUM_CORES;
		     lcore_id += CORE_MASK) {
			ret = rte_power_set_freq(lcore_id, si->next_pstate);
			if (ret < 0) {
				RTE_LOG(ERR, POWER,
					"Failed to scale frequency of lcore %d.\n",
					lcore_id);
			}
		}
		si->last_scale = get_time_of_day();
		f->pstate = si->next_pstate;
		f->freq = f->freqs[f->pstate];
	}
	si->need_scale = false;
}

void calc_pstate(struct measurement *m, struct freq_info *f,
		 struct scaling_info *si)
{
	// First, calc which frequency is needed for a good CPU utilization
	unsigned int new_freq;
	/// Up scaling with larger margin
	/// Calc inter arrival time from cpu_util
	// new_freq = (unsigned int)(CPU_FREQ(m->inter_arrival_time) * 1e-3);
	new_freq = (unsigned int)(CPU_FREQ(m->wma_cpu_util, f->freq));

	// Now, choose the next highest frequency
	if (new_freq >= f->freqs[0]) {
		si->next_pstate = 0; // p-state for Turbo Boost
	} else {
		int freq_idx;
		for (freq_idx = f->num_freqs - 1; freq_idx > 0; freq_idx--) {
			if (f->freqs[freq_idx] >= new_freq) {
				si->next_pstate = freq_idx;
				break;
			}
		}
	}
	printf("New freq: %d, p-state: %d\n", new_freq, si->next_pstate);
}

void check_traffic_trends(struct measurement *m, struct scaling_info *si)
{
	if (m->empty_cnt > MAX_EMPTY_CNT) {
		si->scale_to_min = true; /// for c1 later
	}
	if (m->wma_cpu_util > (m->sma_cpu_util + m->sma_std_err)) {
		si->up_trend = true;
		si->down_trend = false;
	}
	if (m->wma_cpu_util <
	    (m->sma_cpu_util - (m->sma_std_err * TINTERVAL))) {
		si->down_trend = true;
		si->up_trend = false;
	}
}

void check_frequency_scaling(struct measurement *m,
			     __attribute__((unused)) struct freq_info *f,
			     struct scaling_info *si)
{
	if (m->wma_cpu_util > HARD_UP_THRESHOLD) {
		si->scale_up_cnt += HARD_INCREASE;
		si->scale_down_cnt = 0;
		if (si->scale_up_cnt > COUNTER_THRESH) {
			si->need_scale = true;
			si->scale_up_cnt = 0;
		}
		/// Check for up trend first and then against lower threshold
	} else if (m->wma_cpu_util > UTIL_THRESHOLD_UP) {
		si->scale_down_cnt = 0;
		if (si->up_trend) {
			si->scale_up_cnt += TREND_INCREASE;
		} else {
			si->scale_up_cnt += 1;
		}
		if (si->scale_up_cnt >= COUNTER_THRESH) {
			si->need_scale = true;
			si->scale_up_cnt = 0;
		}
	} else if (m->wma_cpu_util < HARD_DOWN_THRESHOLD) {
		si->scale_down_cnt += HARD_DECREASE;
		si->scale_up_cnt = 0;
		if (si->scale_down_cnt > COUNTER_THRESH) {
			si->need_scale = true;
			si->scale_down_cnt = 0;
		}
	} else if (m->wma_cpu_util < UTIL_THRESHOLD_DOWN) {
		si->scale_up_cnt = 0;
		if (si->down_trend) {
			si->scale_down_cnt += TREND_DECREASE;
		} else {
			si->scale_down_cnt += 1;
		}
		if (si->scale_down_cnt > COUNTER_THRESH) {
			si->need_scale = true;
			si->scale_down_cnt = 0;
		}
	}
}

void check_feedback(struct traffic_stats *ts, struct feedback_info *fb,
		    struct scaling_info *si)
{
	// // @0 -> ingress packets, @1 -> egress packets
	// fb->delta_packets =
	// ts[0].total_packets - ts[1].total_packets - fb->packet_offset;

	printf("delta packets: %d\n", fb->delta_packets);
	if (si->empty_cnt > MAX_EMPTY_CNT) {
		si->scale_to_min = true;
	} else if (fb->delta_packets < D_PKT_DOWN_THRESH) {
		si->scale_down_cnt += TREND_DECREASE;
		si->scale_up_cnt = 0;
		if (si->scale_down_cnt > COUNTER_THRESH) {
			fb->freq_down = true;
		}
	} else if (fb->delta_packets > D_PKT_UP_THRESH) {
		if (fb->delta_packets > HARD_D_PKT_UP_THRESH) {
			si->scale_up_cnt += HARD_INCREASE;
		} else {
			si->scale_up_cnt += TREND_INCREASE;
		}
		si->scale_down_cnt = 0;
		if (si->scale_up_cnt >= COUNTER_THRESH) {
			fb->freq_up = true;
		}
	} else {
		si->scale_down_cnt = 0;
		si->scale_up_cnt = 0;
	}
}

void get_cpu_utilization(struct measurement *m, struct freq_info *f)
{
	if (m->empty_cnt == 0) {
		m->cpu_util[m->idx] =
			CPU_UTIL(m->inter_arrival_time, f->freq * 1e3);
	} else {
		m->cpu_util[m->idx] = 0.0;
	}
	printf("CPU frequency %d kHz\n", f->freq);
	printf("Current CPU utilization: %f\n", m->cpu_util[m->idx]);
}

void calc_sma(struct measurement *m)
{
	int i;
	double sum = 0.0;
	if (m->valid_vals >= NUM_READINGS_SMA) {
		for (i = 0; i < NUM_READINGS_SMA; i++) {
			sum += m->cpu_util[i];
		}
		m->sma_cpu_util = sum / NUM_READINGS_SMA;
		// Sample std
		sum = 0.0;
		for (i = 0; i < NUM_READINGS_SMA; i++) {
			sum += ((m->cpu_util[i] - m->sma_cpu_util) *
				(m->cpu_util[i] - m->sma_cpu_util));
		}
		// m->sma_std_err = (sqrt(sum / (NUM_READINGS_SMA - 1)) *
		//   (1 + (1 / (2 * NUM_READINGS_SMA))));
		m->sma_std_err =
			(sqrt(sum / (NUM_READINGS_SMA)) * /// flex a bit;)
			 (1 + (1 / (2 * NUM_READINGS_SMA))));
	} else {
		for (i = 0; i < m->valid_vals; i++) {
			sum += m->cpu_util[i];
		}
		m->sma_cpu_util = sum / m->valid_vals;
		// Sample std
		sum = 0.0;
		for (i = 0; i < m->valid_vals; i++) {
			sum += ((m->cpu_util[i] - m->sma_cpu_util) *
				(m->cpu_util[i] - m->sma_cpu_util));
		}
		// m->sma_std_err = (sqrt(sum / (m->valid_vals - 1)) *
		//   (1 + (1 / (2 * m->valid_vals))));
		m->sma_std_err = (sqrt(sum / (m->valid_vals)) * /// here as well
				  (1 + (1 / (2 * m->valid_vals))));
	}
	printf("SMA: %f, std error: %1.15f\n", m->sma_cpu_util, m->sma_std_err);
}

void calc_wma(struct measurement *m)
{
	int i;
	double sum = 0.0;
	if (m->valid_vals >= NUM_READINGS_WMA) {
		for (i = 0; i < 5; i++) {
			if ((m->idx - i) >= 0) {
				sum += m->cpu_util[m->idx - i] *
				       (NUM_READINGS_WMA - i);
			} else {
				int idx = 10 + (m->idx - i);
				sum += m->cpu_util[idx] *
				       (NUM_READINGS_WMA - i);
			}
		}
		m->wma_cpu_util =
			sum / ((NUM_READINGS_WMA * (NUM_READINGS_WMA + 1)) / 2);
	} else {
		for (i = 0; i < m->valid_vals; i++) {
			sum += m->cpu_util[m->idx - i] * (m->valid_vals - i);
		}
		m->wma_cpu_util =
			sum / ((m->valid_vals * (m->valid_vals + 1)) / 2);
	}
	printf("WMA: %f\n", m->wma_cpu_util);
}

void restore_last_stream_settings(struct last_stream_settings *lss,
				  struct freq_info *f, struct scaling_info *si)
{
	/// Make a quick check against the current cpu utilization to
	/// see if the same frequency is needed
	// set_c1("on");
	// f->pstate = rte_power_get_freq(CORE_OFFSET);
	// f->freq = f->freqs[f->pstate];
	// printf("Frequency %d and p-state %d after wake up\n", f->freq,
	//    f->pstate);
	/// Do we need to scale up after c1?
	si->next_pstate = lss->last_pstate;
	set_pstate(f, si);
	si->restore_settings = false;
}

void calc_traffic_stats(struct measurement *m, struct record *r,
			struct record *p, struct traffic_stats *t_s,
			struct scaling_info *si)
{
	t_s->period = calc_period(r, p);
	t_s->total_packets = r->total.rx_packets;
	t_s->delta_packets = r->total.rx_packets - p->total.rx_packets;
	t_s->pps = t_s->delta_packets / t_s->period;

	if (t_s->delta_packets > 0) {
		if (m->had_first_packet) {
			m->inter_arrival_time =
				(r->total.rx_time - p->total.rx_time) /
				(t_s->delta_packets * 1e9);
			t_s->pps = 1 / m->inter_arrival_time;
			m->empty_cnt = 0,
			m->idx = m->valid_vals % /// Maybe change to valid_vals
				 m->min_cnts; /// Remove min_cnts and use macro
			m->cnt += 1;
			m->valid_vals += 1;

			// was this the first packet after an isg?
			if (si->scaled_to_min) {
				/// Maybe tweak idx and valid_vals here to avoid
				/// extra flag and checking. If restore does checking
				/// on frequency
				printf("Set restore flag\n");
				si->scaled_to_min = false;
				si->restore_settings = true;
			}
		} else {
			m->had_first_packet = true;
			g_csv_saved_stream = false; /// global
			// set_c1("on"); /// Doesn't belong here, just for measurements
		}
	} else if (t_s->delta_packets == 0 && m->had_first_packet) {
		m->empty_cnt += 1; /// Reset if next poll brings a packet
		m->inter_arrival_time = 0.0;
		m->cnt += 1;
		if (!si->scaled_to_min) {
			m->idx = m->valid_vals %
				 m->min_cnts; /// Remove min_cnts and use macro
			m->valid_vals += 1;
		}
	}
}

void get_feedback_stats(struct traffic_stats *ts, struct feedback_info *fb,
			struct record *r, struct record *p)
{
	// ts->period = calc_period(r, p);
	// ts->total_packets = r->total.rx_packets;
	// ts->delta_packets = r->total.rx_packets - p->total.rx_packets;
	// ts->pps = ts->delta_packets / ts->period;

	ts[1].period = calc_period(r, p);
	ts[1].total_packets = r->total.rx_packets;
	ts[1].delta_packets = r->total.rx_packets - p->total.rx_packets;
	ts[1].pps = ts[1].delta_packets / ts[1].period;

	// @0 -> ingress packets, @1 -> egress packets
	fb->delta_packets =
		ts[0].total_packets - ts[1].total_packets - fb->packet_offset;
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
