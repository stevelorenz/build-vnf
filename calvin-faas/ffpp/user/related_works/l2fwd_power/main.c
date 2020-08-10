/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 *
 * About: A modified l2fwd sample application with the power management (PM)
 * mechanisms ported from l3fwd-power sample application.
 *
 * Differences from the PM used in l3fwd-power:
 *
 * - The mapping between lcores, ports and queues are hard-coded instead of
 *   being configurable with --config argument.
 *   So except for the main core, each worker cores handles the ONLY first queue
 *   of its port.
 *
 * - Currently, only APP_MODE_LEGACY is implemented. veth interface does not
 *   support interrupt registration and RX descriptor counting.
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_udp.h>

#include <rte_power.h>
#include <rte_spinlock.h>
#include <rte_power_empty_poll.h>
#include <rte_metrics.h>

#include <ffpp/aes.h>

static volatile bool force_quit;

/* MAC updating enabled by default */
static int mac_updating = 1;

// Disable Crypto function by default.
static bool crypto_enabled = false;
static uint16_t crypto_number = 1;

#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1
#define MAX_MTU_SIZE 1500

// Rx burst read at maximum 32 entries per iteration.
#define MAX_PKT_BURST 4
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* ethernet addresses of ports */
static struct rte_ether_addr l2fwd_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
static uint32_t l2fwd_enabled_port_mask = 0;

/* list of enabled ports */
static uint32_t l2fwd_dst_ports[RTE_MAX_ETHPORTS];

static unsigned int l2fwd_rx_queue_per_lcore = 1;

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

struct rte_mempool *l2fwd_pktmbuf_pool = NULL;

/* Per-port statistics struct */
struct l2fwd_port_statistics {
	uint64_t tx;
	uint64_t rx;
	uint64_t dropped;
} __rte_cache_aligned;
struct l2fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];

// Power management related
// -----------------------------------------------------------------------------

enum app_mode {
	APP_MODE_NO_PM = 0,
	APP_MODE_LEGACY = 1,
	APP_MODE_EMPTY_POLL = 2,
};

// INFO: Legacy mode is used since empty poll require access to the hardware
// interfaces.
enum app_mode app_mode = APP_MODE_LEGACY;

#define MAX_TIMER_PERIOD 86400 /* 1 day max */
/* A tsc-based timer responsible for triggering statistics printout */
static uint64_t timer_period = 10; /* default period is 10 seconds */

static rte_spinlock_t locks[RTE_MAX_ETHPORTS];

// 100 ms interval
#define TIMER_NUMBER_PER_SECOND 10
// 10000 us
#define SCALING_PERIOD (1000000 / TIMER_NUMBER_PER_SECOND)
#define SCALING_DOWN_TIME_RATIO_THRESHOLD 0.25

enum freq_scale_up_hint {
	FREQ_LOWER = -1,
	FREQ_CURRENT = 0,
	FREQ_HIGHER = 1,
	FREQ_HIGHEST = 2
};

// Stats used for PM of each port.
#define MIN_ZERO_POLL_COUNT 10

struct lcore_pm_conf {
	uint16_t port_id;
	uint8_t queue_id;
	enum freq_scale_up_hint freq_scale_up_hint;
	uint32_t zero_rx_packet_count;
	uint32_t idle_hint;
} __rte_cache_aligned;

struct lcore_stats {
	/* total sleep time in ms since last frequency scaling down */
	uint32_t sleep_time;
	/* number of long sleep recently */
	uint32_t nb_long_sleep;
	/* freq. scaling up trend */
	uint32_t trend;
	/* total packet processed recently */
	uint64_t nb_rx_processed;
	/* total iterations looped recently */
	uint64_t nb_iteration_looped;
	/*
	 * Represents empty and non empty polls
	 * of rte_eth_rx_burst();
	 * ep_nep[0] holds non empty polls
	 * i.e. 0 < nb_rx <= MAX_BURST
	 * ep_nep[1] holds empty polls.
	 * i.e. nb_rx == 0
	 */
	uint64_t ep_nep[2];
	/*
	 * Represents full and empty+partial
	 * polls of rte_eth_rx_burst();
	 * ep_nep[0] holds empty+partial polls.
	 * i.e. 0 <= nb_rx < MAX_BURST
	 * ep_nep[1] holds full polls
	 * i.e. nb_rx == MAX_BURST
	 */
	uint64_t fp_nfp[2];
} __rte_cache_aligned;

static struct lcore_stats stats[RTE_MAX_LCORE] __rte_cache_aligned;
static struct rte_timer power_timers[RTE_MAX_LCORE];

/* For empty-poll based PM */
static bool empty_poll_train;
static struct ep_params *ep_params;
static struct ep_policy policy;
static long ep_med_edpi, ep_hgh_edpi;

/*
 * These two thresholds were decided on by running the training algorithm on
 * a 2.5GHz Xeon. These defaults can be overridden by supplying non-zero values
 * for the med_threshold and high_threshold parameters on the command line.
 */
#define EMPTY_POLL_MED_THRESHOLD 350000UL
#define EMPTY_POLL_HGH_THRESHOLD 580000UL

/* (10ms) */
#define INTERVALS_PER_SECOND 100

#define MAX_LCORE_PARAMS 3

struct lcore_params {
	uint16_t port_id;
	uint8_t queue_id;
	uint8_t lcore_id;
} __rte_cache_aligned;

struct lcore_params *lcore_params;
struct lcore_params lcore_params_array[MAX_LCORE_PARAMS];

/*
 * These defaults are using the max frequency index (1), a medium index (9)
 * and a typical low frequency index (14). These can be adjusted to use
 * different indexes using the relevant command line parameters.
 */
static uint8_t freq_tlb[] = { 14, 9, 1 };

// -----------------------------------------------------------------------------

uint8_t aes_key[] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
		      0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
		      0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
		      0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4 };
uint8_t aes_iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

struct AES_ctx aes_ctx;

struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
	uint8_t lcore_id;
	// Assume each lcore only handles one port and one queue.
	struct lcore_pm_conf pm_conf;
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

static __inline__ const char *get_app_mode_string(enum app_mode app_mode)
{
	switch (app_mode) {
	case APP_MODE_NO_PM:
		return "NO_PM";
	case APP_MODE_LEGACY:
		return "LEGACY_MODE";
	case APP_MODE_EMPTY_POLL:
		return "EMPTY_POLL_MODE";
	}
	return "unknown";
}

/* Print out statistics on packets dropped */
static void print_stats(void)
{
	uint64_t total_packets_dropped, total_packets_tx, total_packets_rx;
	unsigned port_id;

	total_packets_dropped = 0;
	total_packets_tx = 0;
	total_packets_rx = 0;

	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H', '\0' };

	/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	printf("\nPort statistics ====================================");

	for (port_id = 0; port_id < RTE_MAX_ETHPORTS; port_id++) {
		/* skip disabled ports */
		if ((l2fwd_enabled_port_mask & (1 << port_id)) == 0)
			continue;
		printf("\nStatistics for port %u ------------------------------"
		       "\nPackets sent: %24" PRIu64
		       "\nPackets received: %20" PRIu64
		       "\nPackets dropped: %21" PRIu64,
		       port_id, port_statistics[port_id].tx,
		       port_statistics[port_id].rx,
		       port_statistics[port_id].dropped);

		total_packets_dropped += port_statistics[port_id].dropped;
		total_packets_tx += port_statistics[port_id].tx;
		total_packets_rx += port_statistics[port_id].rx;
	}
	printf("\nAggregate statistics ==============================="
	       "\nTotal packets sent: %18" PRIu64
	       "\nTotal packets received: %14" PRIu64
	       "\nTotal packets dropped: %15" PRIu64,
	       total_packets_tx, total_packets_rx, total_packets_dropped);
	printf("\n====================================================\n");
}

static inline void l2fwd_mac_updating(struct rte_mbuf *m, unsigned dest_port_id)
{
	struct rte_ether_hdr *eth;
	void *tmp;

	eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

	/* 02:00:00:00:00:xx */
	tmp = &eth->d_addr.addr_bytes[0];
	*((uint64_t *)tmp) = 0x000000000002 + ((uint64_t)dest_port_id << 40);

	/* src addr */
	rte_ether_addr_copy(&l2fwd_ports_eth_addr[dest_port_id], &eth->s_addr);
}

/**
 * @ Encrypt and decrypt the whole IPv4 packet (IP header + IP payload) ONLY for
 * UDP packets.
 */
static inline void l2fwd_aes256_cbc_encrypt_decrypt_ip_udp(struct rte_mbuf *m)
{
	uint16_t i = 0;
	uint8_t *data;
	struct rte_ether_hdr *eth;
	struct rte_ipv4_hdr *iph;
	uint16_t total_length = 0;

	eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
	if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
		return;
	}
	iph = rte_pktmbuf_mtod_offset(m, struct rte_ipv4_hdr *,
				      RTE_ETHER_HDR_LEN);
	if (iph->next_proto_id != IPPROTO_UDP) {
		return;
	}

	total_length = rte_be_to_cpu_16(iph->total_length);
	for (i = 0; i < crypto_number; ++i) {
		data = rte_pktmbuf_mtod_offset(m, uint8_t *, RTE_ETHER_HDR_LEN);
		AES_CBC_encrypt_buffer(&aes_ctx, data, total_length);
		AES_CBC_decrypt_buffer(&aes_ctx, data, total_length);
	}
}

static void l2fwd_simple_forward(struct rte_mbuf *m, unsigned port_id)
{
	unsigned dst_port;
	int sent;
	struct rte_eth_dev_tx_buffer *buffer;
	uint32_t data_len;

	data_len = rte_pktmbuf_data_len(m);
	if (data_len > MAX_MTU_SIZE) {
		RTE_LOG(INFO, L2FWD,
			"Warn: data length: %u is larger than MTU: %u\n",
			data_len, MAX_MTU_SIZE);
		return;
	}
	dst_port = l2fwd_dst_ports[port_id];

	if (unlikely(crypto_enabled == true)) {
		l2fwd_aes256_cbc_encrypt_decrypt_ip_udp(m);
	}

	if (mac_updating)
		l2fwd_mac_updating(m, dst_port);

	buffer = tx_buffer[dst_port];
	sent = rte_eth_tx_buffer(dst_port, 0, buffer, m);
	if (sent)
		port_statistics[dst_port].tx += sent;
}

/**
 * Black magic... Does not work with virtual interfaces yet.
 */
static int event_register(struct lcore_queue_conf *qconf)
{
	uint16_t i;
	uint16_t port_id;
	uint32_t data;
	int ret;

	// Assume the queue ID is always 0.
	for (i = 0; i < qconf->n_rx_port; i++) {
		port_id = qconf->rx_port_list[i];
		data = port_id << CHAR_BIT | 0;
		ret = rte_eth_dev_rx_intr_ctl_q(port_id, 0,
						RTE_EPOLL_PER_THREAD,
						RTE_INTR_EVENT_ADD,
						(void *)((uintptr_t)data));
		if (ret) {
			return ret;
		}
	}

	return 0;
}

#define SUSPEND_THRESHOLD_US 100
#define MINIMUM_SLEEP_TIME_US 1
static inline uint32_t power_idle_heuristic(uint32_t zero_rx_packet_count)
{
	if (zero_rx_packet_count < SUSPEND_THRESHOLD_US) {
		return MINIMUM_SLEEP_TIME_US;
	} else {
		return SUSPEND_THRESHOLD_US;
	}
}

static inline enum freq_scale_up_hint
power_freq_scaleup_heuristic(uint16_t lcore_id, uint16_t port_id,
			     uint16_t queue_id, bool sup_rx_desp_cnt)
{
#define FREQ_GEAR1_RX_PACKET_THRESHOLD MAX_PKT_BURST
#define FREQ_GEAR2_RX_PACKET_THRESHOLD (MAX_PKT_BURST * 2)
#define FREQ_GEAR3_RX_PACKET_THRESHOLD (MAX_PKT_BURST * 3)
#define FREQ_UP_TREND1_ACC 1
#define FREQ_UP_TREND2_ACC 100
#define FREQ_UP_THRESHOLD 10000

	if (sup_rx_desp_cnt == true) {
		uint32_t rxq_count;
		rxq_count = rte_eth_rx_queue_count(port_id, queue_id);
		if (likely(rxq_count > FREQ_GEAR3_RX_PACKET_THRESHOLD)) {
			stats[lcore_id].trend = 0;
			return FREQ_HIGHEST;
		} else if (likely(rxq_count > FREQ_GEAR2_RX_PACKET_THRESHOLD)) {
			stats[lcore_id].trend += FREQ_UP_TREND2_ACC;
		} else if (likely(rxq_count > FREQ_GEAR1_RX_PACKET_THRESHOLD))
			stats[lcore_id].trend += FREQ_UP_TREND1_ACC;

		if (likely(stats[lcore_id].trend > FREQ_UP_THRESHOLD)) {
			stats[lcore_id].trend = 0;
			return FREQ_HIGHER;
		}

	} else {
		// Blind guessing...
		return FREQ_HIGHER;
	}

	return FREQ_HIGHER;
}

/**
 * Callback function to scale the frequency down if required.
 */
static void power_freq_scale_down_cb()
{
	uint64_t hz;
	float sleep_time_ratio;
	uint16_t lcore_id = 0;
	uint16_t cur_lcore_id = rte_lcore_id();

	sleep_time_ratio = (float)(stats[cur_lcore_id].sleep_time) /
			   (float)(SCALING_PERIOD);

	if (sleep_time_ratio >= SCALING_DOWN_TIME_RATIO_THRESHOLD) {
		if (rte_power_freq_down) {
			RTE_LCORE_FOREACH(lcore_id)
			{
				rte_power_freq_down(lcore_id);
			}
		}
	} else if ((uint16_t)(stats[cur_lcore_id].nb_rx_processed /
			      stats[cur_lcore_id].nb_iteration_looped) <
		   MAX_PKT_BURST) {
		// Scale down frequency if average packet per interation is less
		// than full burst size.
		if (rte_power_freq_down) {
			RTE_LCORE_FOREACH(lcore_id)
			{
				rte_power_freq_down(lcore_id);
			}
		}
	}

	// INFO: Reset and start another single timer with current CPU frequency
	// to ensure the period is FIXED.
	hz = rte_get_timer_hz();
	rte_timer_reset_sync(&power_timers[cur_lcore_id],
			     hz / TIMER_NUMBER_PER_SECOND, SINGLE, cur_lcore_id,
			     power_freq_scale_down_cb, NULL);

	stats[cur_lcore_id].nb_rx_processed = 0;
	stats[cur_lcore_id].nb_iteration_looped = 0;
	stats[cur_lcore_id].sleep_time = 0;
}

/* Main processing loop of the legacy power management.
 *
 * - In order to make the scaling work on "old" Intel CPUs, scaling function is
 *   called on ALL RUNNING LCORES (including the master lcore), instead of the
 *   current lcore running the forwarding function.
 *
 * - ISSUE !!!: It runs 10 times faster than the no-pm loop, if the system sleep
 *   function is called.
 * */
static void l2fwd_main_loop_legacy(void)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	int sent;
	unsigned lcore_id;
	unsigned cur_lcore_id;
	uint64_t prev_tsc = 0, diff_tsc, cur_tsc, timer_tsc;
	unsigned i, j, port_id, nb_rx;
	struct lcore_queue_conf *qconf;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) /
				   US_PER_S * BURST_TX_DRAIN_US;
	struct rte_eth_dev_tx_buffer *buffer;

	// PM related.
	uint64_t hz = 0, pm_timer_res_tsc = 0;
	uint64_t prev_tsc_power = 0, diff_tsc_power, cur_tsc_power;
	uint32_t lcore_rx_idle_count = 0;
	int intr_en = 0;

	prev_tsc = 0;
	hz = rte_get_timer_hz();
	// The timer is triggered every 100ms.
	pm_timer_res_tsc = hz / TIMER_NUMBER_PER_SECOND;

	timer_tsc = 0;

	cur_lcore_id = rte_lcore_id();
	qconf = &lcore_queue_conf[cur_lcore_id];

	if (qconf->n_rx_port == 0) {
		RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n",
			cur_lcore_id);
		return;
	}

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", cur_lcore_id);

	for (i = 0; i < qconf->n_rx_port; i++) {
		port_id = qconf->rx_port_list[i];
		RTE_LOG(INFO, L2FWD, " -- lcoreid=%u port_id=%u\n",
			cur_lcore_id, port_id);
	}

	if (event_register(qconf) == 0) {
		intr_en = 1;
	} else {
		RTE_LOG(INFO, L2FWD, "Can not enable RX interrupt.\n");
	}

	// Test if the RX device supports couting number of used
	// descriptors.
	int ret = 0;
	bool sup_rx_desp_cnt = true;
	ret = rte_eth_rx_queue_count(port_id, 0);
	if (ret == -ENOTSUP) {
		printf("The device (port id: %u) does not support descript couting.\n",
		       port_id);
		sup_rx_desp_cnt = false;
	} else if (ret == -EINVAL) {
		rte_exit(EXIT_FAILURE, "Invalid port or queue id.\n");
	}

	while (!force_quit) {
		//rte_delay_us_block(1);
		stats[cur_lcore_id].nb_iteration_looped++;

		cur_tsc = rte_rdtsc();
		// Ignore the cycles in the TX loop.
		cur_tsc_power = cur_tsc;
		/*
		 * TX burst queue drain
		 */
		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {
			for (i = 0; i < qconf->n_rx_port; i++) {
				port_id =
					l2fwd_dst_ports[qconf->rx_port_list[i]];
				buffer = tx_buffer[port_id];

				sent = rte_eth_tx_buffer_flush(port_id, 0,
							       buffer);
				if (sent)
					port_statistics[port_id].tx += sent;
			}

			/* if timer is enabled */
			if (timer_period > 0) {
				/* advance the timer */
				timer_tsc += diff_tsc;

				/* if timer has reached its timeout */
				if (unlikely(timer_tsc >= timer_period)) {
					/* do this only on master core */
					if (cur_lcore_id ==
					    rte_get_master_lcore()) {
						print_stats();
						/* reset the timer */
						timer_tsc = 0;
					}
				}
			}

			prev_tsc = cur_tsc;
		}

		// Triggier the scaling down timer.
		diff_tsc_power = cur_tsc_power - prev_tsc_power;
		if (diff_tsc_power > pm_timer_res_tsc) {
			rte_timer_manage();
			prev_tsc_power = cur_tsc_power;
		}

		/*
		 * Read packet from RX queues
		 */
		lcore_rx_idle_count = 0;
		for (i = 0; i < qconf->n_rx_port; i++) {
			port_id = qconf->rx_port_list[i];
			nb_rx = rte_eth_rx_burst(port_id, 0, pkts_burst,
						 MAX_PKT_BURST);

			port_statistics[port_id].rx += nb_rx;
			stats[cur_lcore_id].nb_rx_processed += nb_rx;

			if (unlikely(nb_rx == 0)) {
				// Try to sleep for a while and hope the CPU
				// will enter C states.
				qconf->pm_conf.zero_rx_packet_count++;
				if (qconf->pm_conf.zero_rx_packet_count <=
				    MIN_ZERO_POLL_COUNT) {
					continue;
				}
				qconf->pm_conf.idle_hint = power_idle_heuristic(
					qconf->pm_conf.zero_rx_packet_count);
				lcore_rx_idle_count++;
			} else {
				qconf->pm_conf.zero_rx_packet_count = 0;
				qconf->pm_conf.freq_scale_up_hint =
					power_freq_scaleup_heuristic(
						cur_lcore_id,
						qconf->pm_conf.port_id,
						qconf->pm_conf.queue_id,
						sup_rx_desp_cnt);
			}

			for (j = 0; j < nb_rx; j++) {
				m = pkts_burst[j];
				rte_prefetch0(rte_pktmbuf_mtod(m, void *));
				l2fwd_simple_forward(m, port_id);
			}
		}

		// Check if PM mechanisms should be performed.
		if (likely(lcore_rx_idle_count == 0)) {
			// Scale up with P-stats.
			if (qconf->pm_conf.freq_scale_up_hint == FREQ_HIGHEST) {
				if (rte_power_freq_max) {
					RTE_LCORE_FOREACH(lcore_id)
					{
						rte_power_freq_max(lcore_id);
					}
				}
			} else if (qconf->pm_conf.freq_scale_up_hint ==
				   FREQ_HIGHER) {
				if (rte_power_freq_up) {
					RTE_LCORE_FOREACH(lcore_id)
					{
						rte_power_freq_up(lcore_id);
					}
				}
			}
		} else {
			// Try to enter C-stats.
			if (qconf->pm_conf.idle_hint < SUSPEND_THRESHOLD_US) {
				// MARK: Execute "pause" instruction to avoid
				// context switching. It is not a spin-lock.
				rte_delay_us(qconf->pm_conf.idle_hint);
			} else {
				// In DPDK's approach, the lcore should suspend
				// until the rx interrupt triggers.
				// The veth interface does not support
				// interrupts. So a system sleep is used here.
				if (intr_en) {
					RTE_LOG(INFO, L2FWD,
						"WARN: interrupt handling is not implemented.");
				}
				// MARK: This line makes the code fast !!!
				// rte_delay_us_sleep(qconf->pm_conf.idle_hint);
			}
			stats[cur_lcore_id].sleep_time +=
				qconf->pm_conf.idle_hint;
		}
	}
}

static void l2fwd_main_loop_no_pm(void)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	int sent;
	unsigned lcore_id;
	uint64_t prev_tsc = 0, diff_tsc, cur_tsc, timer_tsc;
	unsigned i, j, port_id, nb_rx;
	struct lcore_queue_conf *qconf;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) /
				   US_PER_S * BURST_TX_DRAIN_US;
	struct rte_eth_dev_tx_buffer *buffer;

	prev_tsc = 0;
	timer_tsc = 0;

	lcore_id = rte_lcore_id();
	qconf = &lcore_queue_conf[lcore_id];

	if (qconf->n_rx_port == 0) {
		RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n", lcore_id);
		return;
	}

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

	for (i = 0; i < qconf->n_rx_port; i++) {
		port_id = qconf->rx_port_list[i];
		RTE_LOG(INFO, L2FWD, " -- lcoreid=%u port_id=%u\n", lcore_id,
			port_id);
	}

	while (!force_quit) {
		if (rte_power_set_freq) {
			rte_power_set_freq(lcore_id, 1);
		}
		stats[lcore_id].nb_iteration_looped++;

		cur_tsc = rte_rdtsc();
		/*
		 * TX burst queue drain
		 */
		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {
			for (i = 0; i < qconf->n_rx_port; i++) {
				port_id =
					l2fwd_dst_ports[qconf->rx_port_list[i]];
				buffer = tx_buffer[port_id];

				sent = rte_eth_tx_buffer_flush(port_id, 0,
							       buffer);
				if (sent)
					port_statistics[port_id].tx += sent;
			}
			if (rte_power_set_freq) {
				rte_power_set_freq(lcore_id, 1);
			}
			/* if timer is enabled */
			if (timer_period > 0) {
				/* advance the timer */
				timer_tsc += diff_tsc;

				/* if timer has reached its timeout */
				if (unlikely(timer_tsc >= timer_period)) {
					/* do this only on master core */
					if (lcore_id ==
					    rte_get_master_lcore()) {
						print_stats();
						/* reset the timer */
						timer_tsc = 0;
					}
				}
			}

			prev_tsc = cur_tsc;
		}

		/*
		 * Read packet from RX queues
		 */
		for (i = 0; i < qconf->n_rx_port; i++) {
			if (rte_power_set_freq) {
				rte_power_set_freq(lcore_id, 1);
			}
			port_id = qconf->rx_port_list[i];
			nb_rx = rte_eth_rx_burst(port_id, 0, pkts_burst,
						 MAX_PKT_BURST);

			port_statistics[port_id].rx += nb_rx;
			stats[lcore_id].nb_rx_processed += nb_rx;

			for (j = 0; j < nb_rx; j++) {
				m = pkts_burst[j];
				rte_prefetch0(rte_pktmbuf_mtod(m, void *));
				l2fwd_simple_forward(m, port_id);
			}
		}
	}
}

static int l2fwd_launch_one_lcore(void *arg)
{
	enum app_mode *mode;
	mode = (enum app_mode *)arg;
	if (*mode == APP_MODE_LEGACY) {
		RTE_LOG(INFO, L2FWD,
			"Launch l2fwd main loop with legacy mode.\n");
		l2fwd_main_loop_legacy();
	} else if (*mode == APP_MODE_NO_PM) {
		RTE_LOG(INFO, L2FWD,
			"Launch l2fwd main loop without any PM.\n");
		l2fwd_main_loop_no_pm();
	}
	return 0;
}

/* display usage */
static void l2fwd_usage(const char *prgname)
{
	printf("%s [EAL options] -- -p PORTMASK [-q NQ]\n"
	       "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
	       "  -q NQ: number of queue (=ports) per lcore (default is 1)\n"
	       "  -m MODE: Power management mode\n"
	       "       - 0: No power management\n"
	       "       - 1: Legacy power management\n"
	       "  -T PERIOD: statistics will be refreshed each PERIOD seconds (0 to disable, 10 default, 86400 maximum)\n"
	       "  --[no-]mac-updating: Enable or disable MAC addresses updating (enabled by default)\n"
	       "      When enabled:\n"
	       "       - The source MAC address is replaced by the TX port MAC address\n"
	       "       - The destination MAC address is replaced by 02:00:00:00:00:TX_PORT_ID\n",
	       prgname);
}

static int l2fwd_parse_portmask(const char *portmask)
{
	char *end = NULL;
	unsigned long pm;

	/* parse hexadecimal string */
	pm = strtoul(portmask, &end, 16);
	if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (pm == 0)
		return -1;

	return pm;
}

static unsigned int l2fwd_parse_nqueue(const char *q_arg)
{
	char *end = NULL;
	unsigned long n;

	/* parse hexadecimal string */
	n = strtoul(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return 0;
	if (n == 0)
		return 0;
	if (n >= MAX_RX_QUEUE_PER_LCORE)
		return 0;

	return n;
}

static unsigned int l2fwd_parse_mode(const char *q_arg)
{
	char *end = NULL;
	unsigned long n;

	n = strtoul(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;
	if (n == 0) {
		return APP_MODE_NO_PM;
	} else if (n == 1) {
		return APP_MODE_LEGACY;
	} else if (n == 2) {
		return APP_MODE_EMPTY_POLL;
	} else {
		return -1;
	}
}

static unsigned int l2fwd_parse_crypto_arg(const char *q_arg)
{
	char *end = NULL;
	unsigned long n;

	n = strtoul(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return 0;
	if (n == 0)
		return 0;

	return n;
}

static int l2fwd_parse_timer_period(const char *q_arg)
{
	char *end = NULL;
	int n;

	/* parse number string */
	n = strtol(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;
	if (n >= MAX_TIMER_PERIOD)
		return -1;

	return n;
}

static int parse_ep_config(const char *q_arg)
{
	char s[256];
	const char *p = q_arg;
	char *end;
	int num_arg;

	char *str_fld[3];

	int training_flag;
	int med_edpi;
	int hgh_edpi;

	ep_med_edpi = EMPTY_POLL_MED_THRESHOLD;
	ep_hgh_edpi = EMPTY_POLL_MED_THRESHOLD;

	strlcpy(s, p, sizeof(s));

	num_arg = rte_strsplit(s, sizeof(s), str_fld, 3, ',');

	empty_poll_train = false;

	if (num_arg == 0)
		return 0;

	if (num_arg == 3) {
		training_flag = strtoul(str_fld[0], &end, 0);
		med_edpi = strtoul(str_fld[1], &end, 0);
		hgh_edpi = strtoul(str_fld[2], &end, 0);

		if (training_flag == 1) {
			empty_poll_train = true;
			RTE_LOG(INFO, POWER,
				"Run in training mode of empty poll.\n");
		}

		if (med_edpi > 0)
			ep_med_edpi = med_edpi;

		if (med_edpi > 0)
			ep_hgh_edpi = hgh_edpi;

	} else {
		return -1;
	}

	return 0;
}

static const char short_options[] = "p:" /* portmask */
				    "q:" /* number of queues */
				    "T:" /* timer period */
				    "m:" /* power management mode */
				    "h";

#define CMD_LINE_OPT_MAC_UPDATING "mac-updating"
#define CMD_LINE_OPT_NO_MAC_UPDATING "no-mac-updating"

enum {
	/* long options mapped to a short option */

	/* first long only option value must be >= 256, so that we won't
	 * conflict with short options */
	CMD_LINE_OPT_MIN_NUM = 256,
};

static const struct option lgopts[] = {
	{ CMD_LINE_OPT_MAC_UPDATING, no_argument, &mac_updating, 1 },
	{ CMD_LINE_OPT_NO_MAC_UPDATING, no_argument, &mac_updating, 0 },
	{ "empty-poll", 1, 0, 0 },
	{ "enable-crypto", 1, (int *)(&crypto_enabled), true },
	{ "disable-crypto", no_argument, (int *)(&crypto_enabled), false },
	{ NULL, 0, 0, 0 }
};

/* Parse the argument given in the command line of the application */
static int l2fwd_parse_args(int argc, char **argv)
{
	int opt, ret, timer_secs;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];

	argvopt = argv;

	while ((opt = getopt_long(argc, argvopt, short_options, lgopts,
				  &option_index)) != EOF) {
		switch (opt) {
		/* portmask */
		case 'p':
			l2fwd_enabled_port_mask = l2fwd_parse_portmask(optarg);
			if (l2fwd_enabled_port_mask == 0) {
				printf("invalid portmask\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* nqueue */
		case 'q':
			l2fwd_rx_queue_per_lcore = l2fwd_parse_nqueue(optarg);
			if (l2fwd_rx_queue_per_lcore == 0) {
				printf("invalid queue number\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* timer period */
		case 'T':
			timer_secs = l2fwd_parse_timer_period(optarg);
			if (timer_secs < 0) {
				printf("invalid timer period\n");
				l2fwd_usage(prgname);
				return -1;
			}
			timer_period = timer_secs;
			break;

		case 'm':
			ret = l2fwd_parse_mode(optarg);
			if (ret < -1) {
				printf("invalid power management mode.\n");
				l2fwd_usage(prgname);
				return -1;
			}
			app_mode = ret;
			break;

		case 'h':
			l2fwd_usage(prgname);
			break;

		/* long options */
		case 0:
			if (!strncmp(lgopts[option_index].name, "empty-poll",
				     10)) {
				app_mode = APP_MODE_EMPTY_POLL;
				ret = parse_ep_config(optarg);
				if (ret) {
					l2fwd_usage(prgname);
					return -1;
				}
			}

			if (!strncmp(lgopts[option_index].name, "enable-crypto",
				     10)) {
				crypto_enabled = true;
				crypto_number = l2fwd_parse_crypto_arg(optarg);
				if (crypto_number == 0) {
					printf("invalid crypto operation number");
					l2fwd_usage(prgname);
					return -1;
				}
			}
			break;

		default:
			l2fwd_usage(prgname);
			return -1;
		}
	}

	if (optind >= 0)
		argv[optind - 1] = prgname;

	ret = optind - 1;
	optind = 1; /* reset getopt lib */
	return ret;
}

/* Check the link status of all ports in up to 9s, and print them finally */
static void check_all_ports_link_status(uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint16_t port_id;
	uint8_t count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;
	int ret;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		if (force_quit)
			return;
		all_ports_up = 1;
		RTE_ETH_FOREACH_DEV(port_id)
		{
			if (force_quit)
				return;
			if ((port_mask & (1 << port_id)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			ret = rte_eth_link_get_nowait(port_id, &link);
			if (ret < 0) {
				all_ports_up = 0;
				if (print_flag == 1)
					printf("Port %u link get failed: %s\n",
					       port_id, rte_strerror(-ret));
				continue;
			}
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port%d Link Up. Speed %u Mbps - %s\n",
					       port_id, link.link_speed,
					       (link.link_duplex ==
						ETH_LINK_FULL_DUPLEX) ?
						       ("full-duplex") :
						       ("half-duplex\n"));
				else
					printf("Port %d Link Down\n", port_id);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == ETH_LINK_DOWN) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}

static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
		       signum);
		force_quit = true;
	}
}

static int init_power_library(void)
{
	int ret = 0, lcore_id;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id)) {
			/* init power management library */
			ret = rte_power_init(lcore_id);
			if (ret)
				RTE_LOG(ERR, POWER,
					"Library initialization failed on core %u\n",
					lcore_id);
		}
	}
	return ret;
}

static int exit_power_library(void)
{
	int ret = 0, lcore_id;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id)) {
			/* init power management library */
			ret = rte_power_exit(lcore_id);
			if (ret)
				RTE_LOG(ERR, POWER,
					"Library exit failed on core %u\n",
					lcore_id);
		}
	}
	return ret;
}

static void empty_poll_setup_timer(void)
{
	int lcore_id = rte_lcore_id();
	uint64_t hz = rte_get_timer_hz();

	struct ep_params *ep_ptr = ep_params;
	// Number of cycles before the callback is called.
	ep_ptr->interval_ticks = hz / INTERVALS_PER_SECOND;
	// Reset the timer in a loop until success.
	rte_timer_reset_sync(&ep_ptr->timer0, ep_ptr->interval_ticks,
			     PERIODICAL, lcore_id, rte_empty_poll_detection,
			     (void *)ep_ptr);
}

static int launch_timer(unsigned int lcore_id)
{
	RTE_SET_USED(lcore_id);

	if (rte_get_master_lcore() != lcore_id) {
		rte_exit(EXIT_FAILURE,
			 "Timer on lcore:%d which is not a master core:%d\n",
			 lcore_id, rte_get_master_lcore());
	}
	RTE_LOG(INFO, POWER, "Launch the timer\n");
	empty_poll_setup_timer();

	uint64_t cycles_10ms = rte_get_timer_hz() / 100;
	uint64_t prev_tsc = 0, cur_tsc, diff_tsc;
	while (!force_quit) {
		cur_tsc = rte_rdtsc();
		diff_tsc = cur_tsc - prev_tsc;
		if (diff_tsc > cycles_10ms) {
			rte_timer_manage();
			prev_tsc = cur_tsc;
			// CPU frequency keeps changing.
			cycles_10ms = rte_get_timer_hz() / 100;
		}
	}

	RTE_LOG(INFO, POWER, "Timer_subsystem is done\n");

	return 0;
}

void check_lcores(void)
{
	uint16_t lcore_id;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; ++lcore_id) {
		if (rte_lcore_is_enabled(lcore_id)) {
			printf("Check Lcores: Lcore:%d is enabled.\n",
			       lcore_id);
		}
	}
}

void print_empty_poll_params(struct ep_params *ep_ptr)
{
	uint16_t wrk_id;

	for (wrk_id = 0; wrk_id < NUM_NODES; ++wrk_id) {
		printf("The lcore_id of worker %d is %d\n", wrk_id,
		       ep_ptr->wrk_data.wrk_stats[wrk_id].lcore_id);
	}
}

void print_lcore_queue_conf(struct lcore_queue_conf *qconf)
{
	printf("\nLcore queue conf ====================================\n");
	printf("Lcore_id: %d, PM data-> port_id:%u, queue_id:%u, zero_rx_count:%u, idle_hint:%u\n",
	       qconf->lcore_id, qconf->pm_conf.port_id, qconf->pm_conf.queue_id,
	       qconf->pm_conf.zero_rx_packet_count, qconf->pm_conf.idle_hint);
}

int main(int argc, char **argv)
{
	struct lcore_queue_conf *qconf;
	int ret;
	uint16_t nb_ports;
	uint16_t nb_ports_available = 0;
	uint16_t port_id, last_port;
	unsigned lcore_id, rx_lcore_id;
	unsigned nb_ports_in_mask = 0;
	unsigned int nb_lcores = 0;
	unsigned int nb_mbufs;
	uint64_t hz;

	/* init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* init RTE timer library to be used late */
	rte_timer_subsystem_init();

	/* parse application arguments (after the EAL ones) */
	ret = l2fwd_parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid L2FWD arguments\n");

	RTE_LOG(INFO, POWER, "Current working mode: %s\n",
		get_app_mode_string(app_mode));

	if (app_mode == APP_MODE_EMPTY_POLL) {
		rte_exit(EXIT_FAILURE,
			 "Empty mode is currently not fully implemented.\n");
	}

	printf("MAC updating %s\n", mac_updating ? "enabled" : "disabled");

	if (crypto_enabled == true) {
		printf("Crypto operation is enabled. The operation number is %u\n",
		       crypto_number);
		printf("Crypto method: AES256 CBC\n");
		AES_init_ctx_iv(&aes_ctx, aes_key, aes_iv);
	}

	check_lcores();
	init_power_library();

	/* convert to number of cycles */
	timer_period *= rte_get_timer_hz();

	nb_ports = rte_eth_dev_count_avail();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	/* check port mask to possible port mask */
	if (l2fwd_enabled_port_mask & ~((1 << nb_ports) - 1))
		rte_exit(EXIT_FAILURE, "Invalid portmask; possible (0x%x)\n",
			 (1 << nb_ports) - 1);

	/* reset l2fwd_dst_ports */
	for (port_id = 0; port_id < RTE_MAX_ETHPORTS; port_id++)
		l2fwd_dst_ports[port_id] = 0;
	last_port = 0;

	/*
	 * Each logical core is assigned a dedicated TX queue on each port.
	 */
	RTE_ETH_FOREACH_DEV(port_id)
	{
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << port_id)) == 0)
			continue;

		if (nb_ports_in_mask % 2) {
			l2fwd_dst_ports[port_id] = last_port;
			l2fwd_dst_ports[last_port] = port_id;
		} else
			last_port = port_id;

		nb_ports_in_mask++;
	}
	if (nb_ports_in_mask % 2) {
		printf("Notice: odd number of ports in portmask.\n");
		l2fwd_dst_ports[last_port] = last_port;
	}

	rx_lcore_id = 0;
	qconf = NULL;

	/* Initialize the port/queue configuration of each logical core */
	RTE_ETH_FOREACH_DEV(port_id)
	{
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << port_id)) == 0) {
			RTE_LOG(INFO, EAL, "Skipping disabled port %d\n",
				port_id);
			continue;
		}

		/* get the lcore_id for this port */
		while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
		       lcore_queue_conf[rx_lcore_id].n_rx_port ==
			       l2fwd_rx_queue_per_lcore) {
			rx_lcore_id++;
			if (rx_lcore_id >= RTE_MAX_LCORE)
				rte_exit(EXIT_FAILURE, "Not enough cores\n");
		}

		if (qconf != &lcore_queue_conf[rx_lcore_id]) {
			/* Assigned a new logical core in the loop above. */
			qconf = &lcore_queue_conf[rx_lcore_id];
			RTE_LOG(INFO, EAL, "RX lcore ID: %d\n", rx_lcore_id);
			nb_lcores++;
		}

		qconf->rx_port_list[qconf->n_rx_port] = port_id;
		qconf->n_rx_port++;
		qconf->lcore_id = rx_lcore_id;

		qconf->pm_conf.port_id = port_id;
		qconf->pm_conf.queue_id = 0;
		qconf->pm_conf.freq_scale_up_hint = FREQ_CURRENT;
		qconf->pm_conf.zero_rx_packet_count = 0;
		qconf->pm_conf.idle_hint = 0;

		print_lcore_queue_conf(qconf);

		if (app_mode == APP_MODE_LEGACY) {
			RTE_LOG(INFO, POWER,
				"Init and start PM scale down timer for lcore: %u\n",
				rx_lcore_id);
			rte_timer_init(&power_timers[rx_lcore_id]);
			hz = rte_get_timer_hz();
			rte_timer_reset_sync(&power_timers[rx_lcore_id],
					     hz / TIMER_NUMBER_PER_SECOND,
					     SINGLE, rx_lcore_id,
					     power_freq_scale_down_cb, NULL);
		}
	}

	nb_mbufs = RTE_MAX(nb_ports * (nb_rxd + nb_txd + MAX_PKT_BURST +
				       nb_lcores * MEMPOOL_CACHE_SIZE),
			   8192U);

	/* create the mbuf pool */
	l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", nb_mbufs,
						     MEMPOOL_CACHE_SIZE, 0,
						     RTE_MBUF_DEFAULT_BUF_SIZE,
						     rte_socket_id());
	if (l2fwd_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

	/* Initialise each port */
	RTE_ETH_FOREACH_DEV(port_id)
	{
		struct rte_eth_rxconf rxq_conf;
		struct rte_eth_txconf txq_conf;
		struct rte_eth_conf local_port_conf = port_conf;
		struct rte_eth_dev_info dev_info;

		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << port_id)) == 0) {
			printf("Skipping disabled port %u\n", port_id);
			continue;
		}
		nb_ports_available++;

		/* init port */
		printf("Initializing port %u... ", port_id);
		fflush(stdout);

		ret = rte_eth_dev_info_get(port_id, &dev_info);
		if (ret != 0)
			rte_exit(
				EXIT_FAILURE,
				"Error during getting device (port %u) info: %s\n",
				port_id, strerror(-ret));

		if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
			local_port_conf.txmode.offloads |=
				DEV_TX_OFFLOAD_MBUF_FAST_FREE;
		ret = rte_eth_dev_configure(port_id, 1, 1, &local_port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot configure device: err=%d, port=%u\n",
				 ret, port_id);

		ret = rte_eth_dev_adjust_nb_rx_tx_desc(port_id, &nb_rxd,
						       &nb_txd);
		if (ret < 0)
			rte_exit(
				EXIT_FAILURE,
				"Cannot adjust number of descriptors: err=%d, port=%u\n",
				ret, port_id);

		ret = rte_eth_macaddr_get(port_id,
					  &l2fwd_ports_eth_addr[port_id]);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot get MAC address: err=%d, port=%u\n",
				 ret, port_id);

		/* init one RX queue */
		fflush(stdout);
		rxq_conf = dev_info.default_rxconf;
		rxq_conf.offloads = local_port_conf.rxmode.offloads;
		ret = rte_eth_rx_queue_setup(port_id, 0, nb_rxd,
					     rte_eth_dev_socket_id(port_id),
					     &rxq_conf, l2fwd_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				 ret, port_id);

		/* init one TX queue on each port */
		fflush(stdout);
		txq_conf = dev_info.default_txconf;
		txq_conf.offloads = local_port_conf.txmode.offloads;
		ret = rte_eth_tx_queue_setup(port_id, 0, nb_txd,
					     rte_eth_dev_socket_id(port_id),
					     &txq_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "rte_eth_tx_queue_setup:err=%d, port=%u\n",
				 ret, port_id);

		/* Initialize TX buffers */
		tx_buffer[port_id] = rte_zmalloc_socket(
			"tx_buffer", RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
			rte_eth_dev_socket_id(port_id));
		if (tx_buffer[port_id] == NULL)
			rte_exit(EXIT_FAILURE,
				 "Cannot allocate buffer for tx on port %u\n",
				 port_id);

		rte_eth_tx_buffer_init(tx_buffer[port_id], MAX_PKT_BURST);

		ret = rte_eth_tx_buffer_set_err_callback(
			tx_buffer[port_id], rte_eth_tx_buffer_count_callback,
			&port_statistics[port_id].dropped);
		if (ret < 0)
			rte_exit(
				EXIT_FAILURE,
				"Cannot set error callback for tx buffer on port %u\n",
				port_id);

		/* ret = rte_eth_dev_set_ptypes(port_id, RTE_PTYPE_UNKNOWN, NULL, */
		/* 			     0); */
		if (ret < 0)
			printf("Port %u, Failed to disable Ptype parsing\n",
			       port_id);
		/* Start device */
		ret = rte_eth_dev_start(port_id);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "rte_eth_dev_start:err=%d, port=%u\n", ret,
				 port_id);

		printf("done: \n");

		ret = rte_eth_promiscuous_enable(port_id);
		if (ret != 0)
			rte_exit(EXIT_FAILURE,
				 "rte_eth_promiscuous_enable:err=%s, port=%u\n",
				 rte_strerror(-ret), port_id);
		rte_spinlock_init(&locks[port_id]);

		printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
		       port_id, l2fwd_ports_eth_addr[port_id].addr_bytes[0],
		       l2fwd_ports_eth_addr[port_id].addr_bytes[1],
		       l2fwd_ports_eth_addr[port_id].addr_bytes[2],
		       l2fwd_ports_eth_addr[port_id].addr_bytes[3],
		       l2fwd_ports_eth_addr[port_id].addr_bytes[4],
		       l2fwd_ports_eth_addr[port_id].addr_bytes[5]);

		/* initialize port stats */
		memset(&port_statistics, 0, sizeof(port_statistics));
	}

	if (!nb_ports_available) {
		rte_exit(
			EXIT_FAILURE,
			"All available ports are disabled. Please set portmask.\n");
	}

	check_all_ports_link_status(l2fwd_enabled_port_mask);

	if (app_mode == APP_MODE_EMPTY_POLL) {
		if (empty_poll_train) {
			policy.state = TRAINING;
		} else {
			policy.state = MED_NORMAL;
			policy.med_base_edpi = ep_med_edpi;
			policy.hgh_base_edpi = ep_hgh_edpi;
		}

		ret = rte_power_empty_poll_stat_init(&ep_params, freq_tlb,
						     &policy);
		if (ret < 0) {
			rte_exit(EXIT_FAILURE, "Empty poll init failed.\n");
		}
	}

	char resp = 'y';
	printf("Continue to launch the forwarding loop? y/[n]\n");
	resp = fgetc(stdin);
	if (resp == 'y') {
		ret = 0;
		/* launch per-lcore init on every non-main lcore */
		// MARK: If the scaling is required to perform on all cores of a
		// single socket, rte_eal_remote_launch can be used.
		rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, &app_mode,
					 CALL_MASTER);

		if (app_mode == APP_MODE_EMPTY_POLL) {
			launch_timer(rte_lcore_id());
		}

		// SLAVE will be removed in the new DPDK release.
		RTE_LCORE_FOREACH_SLAVE(lcore_id)
		{
			if (rte_eal_wait_lcore(lcore_id) < 0) {
				ret = -1;
				break;
			}
		}
	}

	if (app_mode == APP_MODE_EMPTY_POLL) {
		RTE_LOG(INFO, POWER, "Free empty poll stats.\n");
		rte_power_empty_poll_stat_free();
	}

	RTE_ETH_FOREACH_DEV(port_id)
	{
		if ((l2fwd_enabled_port_mask & (1 << port_id)) == 0)
			continue;
		printf("Closing port %d...", port_id);
		rte_eth_dev_stop(port_id);
		rte_eth_dev_close(port_id);
		printf(" Done\n");
	}

	exit_power_library();

	ret = rte_eal_cleanup();
	if (ret) {
		RTE_LOG(ERR, EAL,
			"Failed to release EAL resources with error code:%d\n",
			ret);
	}

	printf("Bye...\n");

	return ret;
}
