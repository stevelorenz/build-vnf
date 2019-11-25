/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 *
 * About: A modified l2fwd sample application with following changes:
 *
 * - Add the power management algorithm implemented in l3fwd-power sample
 *   application: https://doc.dpdk.org/guides-19.08/sample_app_ug/l3_forward_power_man.html
 *
 * ISSUES:
 *
 * - Empty-poll mode does not work correcly with veth devices, need to chech the
 *   cause of the training error.
 *
 * MARK: This is used to test some features with a network emulator using
 *       virtual network devices.
 *
 * TODO: Add power management in ffpp.
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
#include <rte_timer.h>
#include <rte_power.h>
#include <rte_power_empty_poll.h>

static volatile bool force_quit;

/* MAC updating enabled by default */
static int mac_updating = 1;

#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER2

#define MAX_PKT_BURST 32
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

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

static struct rte_eth_conf port_conf = {
	.rxmode =
		{
			.split_hdr_size = 0,
		},
	.txmode =
		{
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

#define MAX_TIMER_PERIOD 86400 /* 1 day max */
/* A tsc-based timer responsible for triggering statistics printout */
static uint64_t port_stats_timer_period = 0;

/* *** Power Management related ---------------------------------------------- */

enum freq_scale_hint_t {
	FREQ_LOWER = -1,
	FREQ_CURRENT = 0,
	FREQ_HIGHER = 1,
	FREQ_HIGHEST = 2
};

/**
 * The power management stats of the single RX queue of each port.
 */
struct lcore_rx_queue {
	enum freq_scale_hint_t freq_up_hint;
	uint32_t zero_rx_packet_count;
	uint32_t idle_hint;
} __rte_cache_aligned;

/**
 * Stats of each lcore for power management.
 */
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

// Parameters for legacy power management mode
static struct lcore_stats stats[RTE_MAX_LCORE] __rte_cache_aligned;
static struct rte_timer power_timers[RTE_MAX_LCORE];
/* 100 ms interval */
#define TIMER_NUMBER_PER_SECOND 10
/* 100000 us */
#define SCALING_PERIOD (1000000 / TIMER_NUMBER_PER_SECOND)
#define SCALING_DOWN_TIME_RATIO_THRESHOLD 0.25

// Parameters for empty poll
static struct ep_params *ep_params;
static struct ep_policy policy;
static long ep_med_edpi = 0, ep_hgh_edpi = 0;
static bool empty_poll_train;
static bool empty_poll_stop;

/*
 * These two thresholds were decided on by running the training algorithm on
 * a 2.5GHz Xeon. These defaults can be overridden by supplying non-zero values
 * for the med_threshold and high_threshold parameters on the command line.
 */
#define EMPTY_POLL_MED_THRESHOLD 350000UL
#define EMPTY_POLL_HGH_THRESHOLD 580000UL

/*
 * These defaults are using the max frequency index (1), a medium index (9)
 * and a typical low frequency index (14). These can be adjusted to use
 * different indexes using the relevant command line parameters.
 */
static uint8_t freq_tlb[] = { 14, 9, 1 };

/* (10ms) */
#define INTERVALS_PER_SECOND 100

enum appmode {
	APP_MODE_LEGACY = 0,
	APP_MODE_EMPTY_POLL,
};
enum appmode app_mode = APP_MODE_LEGACY;
// -----------------------------------------------------------------------------

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
// The queue configuration of each lcore
// Assumptions:
// - One port has only one RX/TX queue.
struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
	struct lcore_rx_queue rx_queue_list[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

// *** Function signatures
static inline enum freq_scale_hint_t
power_freq_scaleup_heuristic(unsigned lcore_id, uint16_t port_id,
			     uint16_t queue_id);
// -----------------------------------------------------------------------------

// *** Implementation
/* Print out statistics on packets dropped */
__attribute__((unused)) static void print_stats(void)
{
	uint64_t total_packets_dropped, total_packets_tx, total_packets_rx;
	unsigned portid;

	total_packets_dropped = 0;
	total_packets_tx = 0;
	total_packets_rx = 0;

	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H', '\0' };

	/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	printf("\nPort statistics ====================================");

	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
		/* skip disabled ports */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("\nStatistics for port %u ------------------------------"
		       "\nPackets sent: %24" PRIu64
		       "\nPackets received: %20" PRIu64
		       "\nPackets dropped: %21" PRIu64,
		       portid, port_statistics[portid].tx,
		       port_statistics[portid].rx,
		       port_statistics[portid].dropped);

		total_packets_dropped += port_statistics[portid].dropped;
		total_packets_tx += port_statistics[portid].tx;
		total_packets_rx += port_statistics[portid].rx;
	}
	printf("\nAggregate statistics ==============================="
	       "\nTotal packets sent: %18" PRIu64
	       "\nTotal packets received: %14" PRIu64
	       "\nTotal packets dropped: %15" PRIu64,
	       total_packets_tx, total_packets_rx, total_packets_dropped);
	printf("\n====================================================\n");
}

static void l2fwd_mac_updating(struct rte_mbuf *m, unsigned dest_portid)
{
	struct rte_ether_hdr *eth;
	void *tmp;

	eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

	/* 02:00:00:00:00:xx */
	tmp = &eth->d_addr.addr_bytes[0];
	*((uint64_t *)tmp) = 0x000000000002 + ((uint64_t)dest_portid << 40);

	/* src addr */
	rte_ether_addr_copy(&l2fwd_ports_eth_addr[dest_portid], &eth->s_addr);
}

static void l2fwd_simple_forward(struct rte_mbuf *m, unsigned portid)
{
	unsigned dst_port;
	int sent;
	struct rte_eth_dev_tx_buffer *buffer;

	dst_port = l2fwd_dst_ports[portid];

	if (mac_updating)
		l2fwd_mac_updating(m, dst_port);

	buffer = tx_buffer[dst_port];
	sent = rte_eth_tx_buffer(dst_port, 0, buffer, m);
	if (sent)
		port_statistics[dst_port].tx += sent;
}

/* main processing loop
 * MARK: Does not support multiple queues per port.
 * */
static void l2fwd_main_loop(void)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	int sent;
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc, period_tsc_port_stats;
	uint64_t prev_tsc_power, cur_tsc_power, diff_tsc_power,
		period_tsc_power_cb, hz;
	unsigned i, j, portid, nb_rx;
	struct lcore_queue_conf *qconf;
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) /
				   US_PER_S * BURST_TX_DRAIN_US;
	struct rte_eth_dev_tx_buffer *buffer;
	enum freq_scale_hint_t lcore_scaleup_hint;
	uint32_t lcore_rx_idle_count = 0;
	uint32_t lcore_idle_hint = 0;
	struct lcore_rx_queue *rx_queue;

	prev_tsc = 0;
	prev_tsc_power = 0;
	period_tsc_port_stats = 0;
	hz = rte_get_timer_hz();
	period_tsc_power_cb = hz / TIMER_NUMBER_PER_SECOND;

	lcore_id = rte_lcore_id();
	printf("Current lcore id in the l2fwd loop: %u\n", lcore_id);
	qconf = &lcore_queue_conf[lcore_id];

	if (qconf->n_rx_port == 0) {
		RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n", lcore_id);
		return;
	}

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

	for (i = 0; i < qconf->n_rx_port; i++) {
		portid = qconf->rx_port_list[i];
		RTE_LOG(INFO, L2FWD, " -- lcoreid=%u portid=%u\n", lcore_id,
			portid);
	}

	while (!force_quit) {
		stats[lcore_id].nb_iteration_looped++;

		cur_tsc = rte_rdtsc();
		cur_tsc_power = cur_tsc;

		/*
		 * TX burst queue drain
		 */
		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {
			for (i = 0; i < qconf->n_rx_port; i++) {
				portid =
					l2fwd_dst_ports[qconf->rx_port_list[i]];
				buffer = tx_buffer[portid];

				sent = rte_eth_tx_buffer_flush(portid, 0,
							       buffer);
				if (sent)
					port_statistics[portid].tx += sent;
			}

			/* if port stats timer is enabled */
			if (port_stats_timer_period > 0) {
				/* advance the timer */
				period_tsc_port_stats += diff_tsc;

				/* if timer has reached its timeout */
				if (unlikely(period_tsc_port_stats >=
					     port_stats_timer_period)) {
					/* do this only on master core */
					if (lcore_id ==
					    rte_get_master_lcore()) {
						/* print_stats(); */
						/* reset the timer */
						period_tsc_port_stats = 0;
					}
				}
			}

			prev_tsc = cur_tsc;
		}

		diff_tsc_power = cur_tsc_power - prev_tsc_power;
		/* Execute scale down timer when needed. */
		if (diff_tsc_power > period_tsc_power_cb) {
			rte_timer_manage();
			prev_tsc_power = cur_tsc_power;
		}

		/*
		 * Read packet from RX queues
		 */
		lcore_scaleup_hint = FREQ_CURRENT;
		lcore_rx_idle_count = 0;
		for (i = 0; i < qconf->n_rx_port; i++) {
			portid = qconf->rx_port_list[i];
			rx_queue = &(qconf->rx_queue_list[i]);
			rx_queue->idle_hint = 0;
			nb_rx = rte_eth_rx_burst(portid, 0, pkts_burst,
						 MAX_PKT_BURST);
			port_statistics[portid].rx += nb_rx;

			stats[lcore_id].nb_rx_processed += nb_rx;
			if (unlikely(nb_rx == 0)) {
				// The IDLE heuristic for sleeping should run here.
				// It's leider unimplemented yet.
				rx_queue->zero_rx_packet_count++;
			} else {
				rx_queue->zero_rx_packet_count = 0;

				/** Do not scale up frequency immediately as
				 * user to kernel space communication is costly
				 * which might impact packet I/O for received
				 * packets.
				 */
				rx_queue->freq_up_hint =
					power_freq_scaleup_heuristic(lcore_id,
								     portid, 0);
				RTE_LOG(DEBUG, POWER,
					"Current frequency scaleup hint:%d\n",
					rx_queue->freq_up_hint);
			}

			for (j = 0; j < nb_rx; j++) {
				m = pkts_burst[j];
				rte_prefetch0(rte_pktmbuf_mtod(m, void *));
				l2fwd_simple_forward(m, portid);
			}

			// Scale up frequency if needed.
			if (rx_queue->freq_up_hint > lcore_scaleup_hint) {
				lcore_scaleup_hint = rx_queue->freq_up_hint;
			}
			if (lcore_scaleup_hint == FREQ_HIGHEST) {
				if (rte_power_freq_max)
					RTE_LOG(DEBUG, POWER,
						"Scale up to max frequency\n");
				rte_power_freq_max(lcore_id);
			} else if (lcore_scaleup_hint == FREQ_HIGHER) {
				if (rte_power_freq_up)
					RTE_LOG(DEBUG, POWER,
						"Scale up to higher frequency\n");
				rte_power_freq_up(lcore_id);
			}
		}
	}
}

static int l2fwd_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	l2fwd_main_loop();
	return 0;
}

/* display usage */
static void l2fwd_usage(const char *prgname)
{
	printf("%s [EAL options] -- -p PORTMASK [-q NQ]\n"
	       "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
	       "  -q NQ: number of queue (=ports) per lcore (default is 1)\n"
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

static int l2fwd_parse_ep_config(const char *q_arg)
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

		if (training_flag == 1)
			RTE_LOG(DEBUG, L2FWD, "Enable training mode.\n");
		empty_poll_train = true;

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
				    "e:" /* empty poll mode*/
				    "q:" /* number of queues */
				    "T:" /* timer period */
	;

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
			port_stats_timer_period = timer_secs;
			break;

		case 'e':
			ret = l2fwd_parse_ep_config(optarg);
			if (ret) {
				printf("Invalid empty poll config.\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* long options */
		case 0:
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
	uint16_t portid;
	uint8_t count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		if (force_quit)
			return;
		all_ports_up = 1;
		RTE_ETH_FOREACH_DEV(portid)
		{
			if (force_quit)
				return;
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port%d Link Up. Speed %u Mbps - %s\n",
					       portid, link.link_speed,
					       (link.link_duplex ==
						ETH_LINK_FULL_DUPLEX) ?
						       ("full-duplex") :
						       ("half-duplex\n"));
				else
					printf("Port %d Link Down\n", portid);
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

// MARK: This can run only on bare-mental.
static int init_power_library(void)
{
	int ret = 0, lcore_id;
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id)) {
			/* init power management library */
			ret = rte_power_init(lcore_id);
			if (ret)
				RTE_LOG(ERR, L2FWD,
					"Library initialization failed on core %u\n",
					lcore_id);
		}
	}
	RTE_LOG(DEBUG, POWER, "The inited power management env is %d\n",
		rte_power_get_env());
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
				RTE_LOG(INFO, L2FWD,
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

static void empty_poll_setup_timer(void)
{
	int lcore_id = rte_lcore_id();
	uint64_t hz = rte_get_timer_hz();

	struct ep_params *ep_ptr = ep_params;

	ep_ptr->interval_ticks = hz / INTERVALS_PER_SECOND;

	rte_timer_reset_sync(&ep_ptr->timer0, ep_ptr->interval_ticks,
			     PERIODICAL, lcore_id, rte_empty_poll_detection,
			     (void *)ep_ptr);
}

static int launch_timer(unsigned int lcore_id)
{
	int64_t prev_tsc = 0, cur_tsc, diff_tsc, cycles_10ms;

	RTE_SET_USED(lcore_id);

	if (rte_get_master_lcore() != lcore_id) {
		rte_panic("timer on lcore:%d which is not master core:%d\n",
			  lcore_id, rte_get_master_lcore());
	}

	RTE_LOG(INFO, POWER, "Bring up the Timer\n");

	empty_poll_setup_timer();

	cycles_10ms = rte_get_timer_hz() / 100;

	while (!force_quit) {
		cur_tsc = rte_rdtsc();
		diff_tsc = cur_tsc - prev_tsc;
		if (diff_tsc > cycles_10ms) {
			rte_timer_manage();
			prev_tsc = cur_tsc;
			cycles_10ms = rte_get_timer_hz() / 100;
		}
	}

	RTE_LOG(INFO, POWER, "Timer_subsystem is done\n");

	return 0;
}

/**
 * Heuristic algorithm to scale up CPU frequency.
 *
 * @param lcore_id
 * @param port_id
 * @param queue_id
 *
 * @return
 */
static inline enum freq_scale_hint_t
power_freq_scaleup_heuristic(unsigned lcore_id, uint16_t port_id,
			     uint16_t queue_id)
{
	uint32_t rxq_count = rte_eth_rx_queue_count(port_id, queue_id);
/**
 * HW Rx queue size is 128 by default, Rx burst read at maximum 32 entries
 * per iteration
 */
#define FREQ_GEAR1_RX_PACKET_THRESHOLD MAX_PKT_BURST
#define FREQ_GEAR2_RX_PACKET_THRESHOLD (MAX_PKT_BURST * 2)
#define FREQ_GEAR3_RX_PACKET_THRESHOLD (MAX_PKT_BURST * 3)
#define FREQ_UP_TREND1_ACC 1
#define FREQ_UP_TREND2_ACC 100
#define FREQ_UP_THRESHOLD 10000

	if (likely(rxq_count > FREQ_GEAR3_RX_PACKET_THRESHOLD)) {
		stats[lcore_id].trend = 0;
		return FREQ_HIGHEST;
	} else if (likely(rxq_count > FREQ_GEAR2_RX_PACKET_THRESHOLD))
		stats[lcore_id].trend += FREQ_UP_TREND2_ACC;
	else if (likely(rxq_count > FREQ_GEAR1_RX_PACKET_THRESHOLD))
		stats[lcore_id].trend += FREQ_UP_TREND1_ACC;

	if (likely(stats[lcore_id].trend > FREQ_UP_THRESHOLD)) {
		stats[lcore_id].trend = 0;
		return FREQ_HIGHER;
	}

	return FREQ_CURRENT;
}

/*  Freqency scale down timer callback */
static void power_timer_cb(__attribute__((unused)) struct rte_timer *tim,
			   __attribute__((unused)) void *arg)
{
	uint64_t hz;
	float sleep_time_ratio;
	unsigned lcore_id = rte_lcore_id();

	/* accumulate total execution time in us when callback is invoked */
	sleep_time_ratio =
		(float)(stats[lcore_id].sleep_time) / (float)SCALING_PERIOD;
	RTE_LOG(DEBUG, POWER,
		"Scale down timer is triggered on lcore: %u\n"
		"Lcore stats: sleep_time_ratio: %f, iterations_looped:%lu, nb_rx_processed:%lu\n"
		"sleep_time_ratio:%f , threshold:%f,"
		"current frequency index:%u"
		"\n",
		lcore_id, sleep_time_ratio, stats[lcore_id].nb_iteration_looped,
		stats[lcore_id].nb_rx_processed, sleep_time_ratio,
		SCALING_DOWN_TIME_RATIO_THRESHOLD,
		rte_power_get_freq(lcore_id));

	/**
	 * check whether need to scale down frequency a step if it sleep a lot.
	 */
	if (sleep_time_ratio >= SCALING_DOWN_TIME_RATIO_THRESHOLD) {
		if (rte_power_freq_down)
			rte_power_freq_down(lcore_id);
	} else if ((unsigned)(stats[lcore_id].nb_rx_processed /
			      stats[lcore_id].nb_iteration_looped) <
		   MAX_PKT_BURST) {
		/**
		 * scale down a step if average packet per iteration less
		 * than expectation.
		 */
		if (rte_power_freq_down)
			rte_power_freq_down(lcore_id);
	} else {
		RTE_LOG(DEBUG, POWER, "Frequency is not scaled down.\n");
	}
	/**
	 * initialize another timer according to current frequency to ensure
	 * timer interval is relatively fixed.
	 */
	hz = rte_get_timer_hz();
	rte_timer_reset(&power_timers[lcore_id], hz / TIMER_NUMBER_PER_SECOND,
			SINGLE, lcore_id, power_timer_cb, NULL);

	stats[lcore_id].nb_rx_processed = 0;
	stats[lcore_id].nb_iteration_looped = 0;

	stats[lcore_id].sleep_time = 0;
}

int main(int argc, char **argv)
{
	struct lcore_queue_conf *qconf;
	int ret;
	uint16_t nb_ports;
	uint16_t nb_ports_available = 0;
	uint16_t portid, last_port;
	uint64_t hz;
	unsigned lcore_id, rx_lcore_id;
	unsigned nb_ports_in_mask = 0;
	unsigned int nb_lcores = 0;
	unsigned int nb_mbufs;

	/* init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;

	/* Init RTE timer library */
	rte_timer_subsystem_init();

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* parse application arguments (after the EAL ones) */
	ret = l2fwd_parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid L2FWD arguments\n");

	printf("MAC updating %s\n", mac_updating ? "enabled" : "disabled");

	/* Init power management library */
	if (init_power_library()) {
		rte_exit(EXIT_FAILURE, "Init power library failed.\n");
	}

	check_lcore_power_caps();

	printf("*** The ID of the master core: %u\n", rte_get_master_lcore());

	/* convert to number of cycles */
	port_stats_timer_period *= rte_get_timer_hz();

	nb_ports = rte_eth_dev_count_avail();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	/* check port mask to possible port mask */
	if (l2fwd_enabled_port_mask & ~((1 << nb_ports) - 1))
		rte_exit(EXIT_FAILURE, "Invalid portmask; possible (0x%x)\n",
			 (1 << nb_ports) - 1);

	/* reset l2fwd_dst_ports */
	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
		l2fwd_dst_ports[portid] = 0;
	last_port = 0;

	/*
	 * Each logical core is assigned a dedicated TX queue on each port.
	 */
	RTE_ETH_FOREACH_DEV(portid)
	{
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;

		if (nb_ports_in_mask % 2) {
			l2fwd_dst_ports[portid] = last_port;
			l2fwd_dst_ports[last_port] = portid;
		} else
			last_port = portid;

		nb_ports_in_mask++;
	}
	if (nb_ports_in_mask % 2) {
		printf("Notice: odd number of ports in portmask.\n");
		l2fwd_dst_ports[last_port] = last_port;
	}

	rx_lcore_id = 1;
	qconf = NULL;

	/* Initialize the port/queue configuration of each logical core */
	RTE_ETH_FOREACH_DEV(portid)
	{
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;

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
			nb_lcores++;
		}

		qconf->rx_port_list[qconf->n_rx_port] = portid;
		qconf->n_rx_port++;
		printf("Lcore %u: RX port %u\n", rx_lcore_id, portid);
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

	/* Initial timer for legacy mode */
	if (app_mode == APP_MODE_LEGACY) {
		for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
			if (rte_lcore_is_enabled(lcore_id)) {
				RTE_LOG(INFO, POWER,
					"Init frequency scaling-down callback on lcore: %u\n",
					lcore_id);
				/* init timer structures for each enabled lcore */
				rte_timer_init(&power_timers[lcore_id]);
				hz = rte_get_timer_hz();
				rte_timer_reset(&power_timers[lcore_id],
						hz / TIMER_NUMBER_PER_SECOND,
						SINGLE, lcore_id,
						power_timer_cb, NULL);
			}
		}
	}

	/* Initialise each port */
	RTE_ETH_FOREACH_DEV(portid)
	{
		struct rte_eth_rxconf rxq_conf;
		struct rte_eth_txconf txq_conf;
		struct rte_eth_conf local_port_conf = port_conf;
		struct rte_eth_dev_info dev_info;

		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0) {
			printf("Skipping disabled port %u\n", portid);
			continue;
		}
		nb_ports_available++;

		/* init port */
		printf("Initializing port %u... ", portid);
		fflush(stdout);
		rte_eth_dev_info_get(portid, &dev_info);
		if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
			local_port_conf.txmode.offloads |=
				DEV_TX_OFFLOAD_MBUF_FAST_FREE;
		ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot configure device: err=%d, port=%u\n",
				 ret, portid);

		ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,
						       &nb_txd);
		if (ret < 0)
			rte_exit(
				EXIT_FAILURE,
				"Cannot adjust number of descriptors: err=%d, port=%u\n",
				ret, portid);

		rte_eth_macaddr_get(portid, &l2fwd_ports_eth_addr[portid]);

		/* init one RX queue */
		fflush(stdout);
		rxq_conf = dev_info.default_rxconf;
		rxq_conf.offloads = local_port_conf.rxmode.offloads;
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
					     rte_eth_dev_socket_id(portid),
					     &rxq_conf, l2fwd_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				 ret, portid);

		/* init one TX queue on each port */
		fflush(stdout);
		txq_conf = dev_info.default_txconf;
		txq_conf.offloads = local_port_conf.txmode.offloads;
		ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
					     rte_eth_dev_socket_id(portid),
					     &txq_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "rte_eth_tx_queue_setup:err=%d, port=%u\n",
				 ret, portid);

		/* Initialize TX buffers */
		tx_buffer[portid] = rte_zmalloc_socket(
			"tx_buffer", RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
			rte_eth_dev_socket_id(portid));
		if (tx_buffer[portid] == NULL)
			rte_exit(EXIT_FAILURE,
				 "Cannot allocate buffer for tx on port %u\n",
				 portid);

		rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

		ret = rte_eth_tx_buffer_set_err_callback(
			tx_buffer[portid], rte_eth_tx_buffer_count_callback,
			&port_statistics[portid].dropped);
		if (ret < 0)
			rte_exit(
				EXIT_FAILURE,
				"Cannot set error callback for tx buffer on port %u\n",
				portid);

		/* Start device */
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "rte_eth_dev_start:err=%d, port=%u\n", ret,
				 portid);

		printf("done: \n");

		rte_eth_promiscuous_enable(portid);

		printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
		       portid, l2fwd_ports_eth_addr[portid].addr_bytes[0],
		       l2fwd_ports_eth_addr[portid].addr_bytes[1],
		       l2fwd_ports_eth_addr[portid].addr_bytes[2],
		       l2fwd_ports_eth_addr[portid].addr_bytes[3],
		       l2fwd_ports_eth_addr[portid].addr_bytes[4],
		       l2fwd_ports_eth_addr[portid].addr_bytes[5]);

		/* initialize port stats */
		memset(&port_statistics, 0, sizeof(port_statistics));
	}

	if (!nb_ports_available) {
		rte_exit(
			EXIT_FAILURE,
			"All available ports are disabled. Please set portmask.\n");
	}

	check_all_ports_link_status(l2fwd_enabled_port_mask);

	if (app_mode == APP_MODE_LEGACY) {
		ret = 0;
		/* launch per-lcore init on every lcore */
		rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, NULL,
					 CALL_MASTER);
	} else if (app_mode == APP_MODE_EMPTY_POLL) {
		rte_exit(EXIT_FAILURE,
			 "Empty-poll mode is not fully implemented now.");
		RTE_LOG(INFO, L2FWD, "Empty-poll mode is used.\n");
		if (empty_poll_train) {
			policy.state = TRAINING;
		} else {
			policy.state = MED_NORMAL;
			policy.med_base_edpi = ep_med_edpi;
			policy.hgh_base_edpi = ep_hgh_edpi;
		}

		ret = rte_power_empty_poll_stat_init(&ep_params, freq_tlb,
						     &policy);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "empty poll init failed");
		empty_poll_stop = false;

		ret = 0;

		/* launch per-lcore init on every slave lcore: SKIP_MASTER */
		rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, NULL,
					 SKIP_MASTER);

		launch_timer(rte_lcore_id());
	}

	RTE_LCORE_FOREACH_SLAVE(lcore_id)
	{
		RTE_LOG(INFO, EAL, "Wait for lcore: %u\n", lcore_id);
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}

	RTE_LOG(INFO, EAL, "Run resource cleanups.\n");
	for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
		if (rte_lcore_is_enabled(lcore_id)) {
			ret = rte_power_exit(lcore_id);
			if (ret)
				RTE_LOG(ERR, L2FWD,
					"Library initialization failed on core %u\n",
					lcore_id);
		}
	}

	if (app_mode == APP_MODE_EMPTY_POLL) {
		rte_power_empty_poll_stat_free();
	}

	RTE_ETH_FOREACH_DEV(portid)
	{
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
	printf("Bye...\n");

	return ret;
}
