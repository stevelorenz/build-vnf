/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *    Description:  Append timestamps to all received UDP packets and log processing delays
 *        Version:  0.1
 *          Email:  xianglinks@gmail.com
 *
 *	 SPDX-License-Identifier: BSD-3-Clause
 *   Copyright(c) 2010-2016 Intel Corporation
 *
 *			 TODO:
 *				- Focus on single ingress and egress queue, reduce the code.
 *				- Bind the packet processing to another lcore
 * =====================================================================================
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
#include <time.h>
#include <unistd.h>

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
#include <rte_byteorder.h>
#include <rte_pdump.h>


static volatile bool force_quit;

/* Verbose level */
static int verbose = 0;

/* Enable debug mode */
static int debugging = 0;

/* MAC updating enabled by default */
static int mac_updating = 1;

/* Disable packet capture by default
 * Packet capture is supported by pdump library
 * */
static int packet_capturing = 0;

/* Appending time stamps, default enabled */
static int appending_ts = 1;

/* Burst-based packet processing */
#define MAX_PKT_BURST 32 /* Default maximal received packets each time */
#define BURST_RX_TRY_US 100  /* Try to receive from the RX every X microseconds */
#define RX_LONG_SLEEP_US 1000
#define MAX_BURST_RX_TRY 10

#define BURST_TX_DRAIN_US 100  /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256

/* Ethernet Frames */
#define MAX_FRAM_SIZE 1500

/* Processing delay of frames */
static uint64_t st_proc_tsc = 0;
static uint64_t end_proc_tsc = 0;
static uint64_t proc_tsc = 0;
static double proc_time = 0.0;
static uint64_t nb_udp_dgrams = 0;

/* Source and destination MAC addresses used for sending frames */
static uint32_t dst_mac[ETHER_ADDR_LEN];
static uint32_t src_mac[ETHER_ADDR_LEN];
static struct ether_addr dst_mac_addr;
static struct ether_addr src_mac_addr;

/* Layer 4 Protocols */
#define UDP_HDR_LEN 8
#define MAX_UDP_DATA_LEN 512

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 1024
#define RTE_TEST_TX_DESC_DEFAULT 1024
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* Ethernet addresses of ports */
static struct ether_addr l2fwd_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
static uint32_t l2fwd_enabled_port_mask = 0;

/* list of enabled ports */
static uint32_t l2fwd_dst_ports[RTE_MAX_ETHPORTS];

static unsigned int l2fwd_rx_queue_per_lcore = 1;

#define MAX_RX_QUEUE_PER_LCORE 4
#define MAX_TX_QUEUE_PER_PORT 4


struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
	volatile bool start_rx_flag[MAX_RX_QUEUE_PER_LCORE]; /* A flag indicate the queue should start burst receiving */
	uint16_t rx_burst_try[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;

struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

static struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
		.ignore_offload_bitfield = 1,
		.offloads = DEV_RX_OFFLOAD_CRC_STRIP,
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

/* Memory pool for all mbufs */
struct rte_mempool * l2fwd_pktmbuf_pool = NULL;

/* Per-port statistics struct */
struct l2fwd_port_statistics {
	uint64_t tx;
	uint64_t rx;
	uint64_t dropped;
} __rte_cache_aligned;

struct l2fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];

#define MAX_TIMER_PERIOD 86400 /* 1 day max */

/**
 * @brief Convert Uint32 IP address to x.x.x.x format
 */
	static void
get_ip_str(uint32_t ip, char *ip_addr_str)
{
	ip = ntohl(ip);
	uint8_t octet[4];
	/*char ip_addr_str[16];*/
	for(int i = 0 ; i < 4; i++)
	{
		octet[i] =  ip >> (i * 8);
	}
	sprintf(ip_addr_str, "%d.%d.%d.%d", octet[3], octet[2], octet[1], octet[0]);
}

/**
 * @brief Filter a Ethernet frame, return true if frame match the rules.
 */
	__attribute__((unused)) static int8_t
filter_ether_frame(struct rte_mbuf *m)
{
	struct ether_hdr *ethh;
	ethh = rte_pktmbuf_mtod(m, struct ether_hdr *);
	/* Filter out non-IP packets */
	if (ethh->ether_type != rte_cpu_to_be_16(ETHER_TYPE_IPv4))
		return -1;

	/* Filter out non-UDP packets */
	struct ipv4_hdr *iph;
	iph = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ETHER_HDR_LEN);
	if (iph->next_proto_id != IPPROTO_UDP)
		return -2;

	/* Filter out looping packets with source MAC address of any forwarding
	 * ports  */
	uint8_t i;
	for (i = 0; i < sizeof(l2fwd_ports_eth_addr); ++i) {
		if ( is_same_ether_addr(&(ethh->s_addr), &(l2fwd_ports_eth_addr[i])) )
			return -3;
	}

	return 1;
}


	__attribute__((unused)) static bool
is_ipv4_pkt(struct rte_mbuf *m)
{
	struct ether_hdr *ethh;
	ethh = rte_pktmbuf_mtod(m, struct ether_hdr *);
	if (ethh->ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4)) {
		return true;
	}
	else
		return false;
}

	__attribute__((unused)) static bool
is_udp_dgram(struct rte_mbuf *m)
{
	struct ipv4_hdr *iph;
	iph = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ETHER_HDR_LEN);
	if (iph->next_proto_id == IPPROTO_UDP) {
		return true;
	}
	else
		return false;
}

/**
 * @brief Append timestamps at the end of UDP diagram
 */
	static void
udp_append_ts(struct rte_mbuf *m)
{
	/* Timestamp in microsecond */
	struct timespec tms;
	clock_gettime(CLOCK_REALTIME, &tms);
	uint64_t micros = tms.tv_sec * 1000000;
	micros += tms.tv_nsec / 1000;
	if (tms.tv_nsec % 1000 >= 500) {
		micros++;
	}

	/* Mod UDP payload */
	struct ipv4_hdr *iph;
	iph = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ETHER_HDR_LEN);
	/* Get IHL */
	uint8_t ihl = iph->version_ihl & 0x0F;
	uint16_t iph_len = ihl * 32 / 8;

	struct udp_hdr *udph;
	udph = (struct udp_hdr *) ((char *) iph + iph_len);

	uint16_t data_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;

	/* Append timestamps */
	/* MARK: SHOULD append at the end of the mbuf */
	char *append_data;
	append_data = rte_pktmbuf_append(m, sizeof(micros));
	if (unlikely(!append_data)) {
		rte_exit(EXIT_FAILURE, "Can not append timestamps to the received UDP packet.\n");
	}
	rte_memcpy(append_data, &micros, sizeof(micros));

	data_len = data_len + sizeof(micros);
	/* Update UDP and IP headers */
	udph->dgram_len = rte_cpu_to_be_16(data_len + UDP_HDR_LEN);
	iph->total_length = rte_cpu_to_be_16(
			rte_be_to_cpu_16(iph->total_length) + sizeof(micros)
			);
	udph->dgram_cksum = 0;
	iph->hdr_checksum = 0;

	udph->dgram_cksum = rte_ipv4_udptcp_cksum(iph, udph);
	iph->hdr_checksum = rte_ipv4_cksum(iph);

}

	static void
l2fwd_mac_updating(struct rte_mbuf *m)
{
	struct ether_hdr *eth;
	eth = rte_pktmbuf_mtod(m, struct ether_hdr *);
	/* Update destination and source MAC addr */
	ether_addr_copy(&src_mac_addr, &eth->s_addr);
	ether_addr_copy(&dst_mac_addr, &eth->d_addr);
}

	static void
l2fwd_simple_forward(struct rte_mbuf *m, unsigned portid)
{
	unsigned dst_port;
	int sent;
	struct rte_eth_dev_tx_buffer *buffer;

	dst_port = l2fwd_dst_ports[portid];

	if (mac_updating)
		l2fwd_mac_updating(m);

	buffer = tx_buffer[dst_port];

	sent = rte_eth_tx_buffer(dst_port, 0, buffer, m);

	/* Possible draining processes */
	if (sent){
		RTE_LOG(DEBUG, USER1, "Trigger %d UDP packets drained in the TX buffer\n", sent);
	}
}

/**
 * @brief Log infos of received packets
 */
	__attribute__((unused)) static void
log_rx_pkt(struct rte_mbuf *m)
{
	/* Ethernet header part
	 * The eth is a pointer to the header struct
	 * Call rte_pktmbuf_mtod to move the pointer to the data part in the
	 * mbuf and use ether_hdr to cast it into the Ethernet header type.
	 * */
	printf("--------------------------------------------------------------------------\n");
	printf("Number of segments: %d\n", m->nb_segs);
	struct ether_hdr *eth;
	eth = rte_pktmbuf_mtod(m, struct ether_hdr *);
	/* MAC addresses
	 * 48 bits -> 6 bytes
	 * */
	char mac_addr [ETHER_ADDR_LEN * 2 + 5 + 1];
	uint8_t *addr_ptr;
	addr_ptr = eth->s_addr.addr_bytes;
	/* Convert addr_bytes into string with hexadecimal format */
	snprintf(mac_addr, sizeof(mac_addr), "%02x:%02x:%02x:%02x:%02x:%02x",
			addr_ptr[0], addr_ptr[1], addr_ptr[2], addr_ptr[3], addr_ptr[4], addr_ptr[5]
			);
	printf("SRC MAC: %s\n", mac_addr);

	addr_ptr = eth->d_addr.addr_bytes;
	snprintf(mac_addr, sizeof(mac_addr), "%02x:%02x:%02x:%02x:%02x:%02x",
			addr_ptr[0], addr_ptr[1], addr_ptr[2], addr_ptr[3], addr_ptr[4], addr_ptr[5]
			);
	printf("DST MAC: %s\n", mac_addr);

	/* IP header */
	struct ipv4_hdr *iphdr;
	/* Check mbuf API: Move the pointer to an offset into the data in the
	 * mbuf */
	iphdr = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr *, ETHER_HDR_LEN);
	char ip_addr_str[16];
	get_ip_str(iphdr->src_addr, ip_addr_str);
	printf("SRC IP address: %s\n",ip_addr_str);
	get_ip_str(iphdr->dst_addr, ip_addr_str);
	printf("DST IP address: %s\n",ip_addr_str);
	printf("Total length: %d\n", iphdr->total_length);
	printf("Packet ID: %d\n", iphdr->packet_id);

	/* UDP header */

	printf("--------------------------------------------------------------------------\n");
}

/* Check Link Status */

/* Main forwarding and processing loop
 *
 * MARK: This function handles packet receiving, processing and transmitting.
 * */
	static void
l2fwd_main_loop(void)
{
	/* MARK: Packets are read in a burst of size MAX_PKT_BURST
	 * This is also a tunable parameter for latency
	 * */
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	unsigned lcore_id;
	unsigned i, j, portid, nb_rx;
	uint8_t filter_ret = 0;
	struct lcore_queue_conf *qconf;
	uint64_t prev_tsc, diff_tsc, cur_tsc;
	uint16_t sent;

	/* MARK: Why there is a US_PER_S - 1 is added? Maybe check jiffes */
	const uint64_t drain_tsc = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;
	struct rte_eth_dev_tx_buffer *buffer;

	lcore_id = rte_lcore_id();
	qconf = &lcore_queue_conf[lcore_id];  /* queue configuration */

	if (qconf->n_rx_port == 0) {
		RTE_LOG(INFO, USER1, "lcore %u has nothing to do\n", lcore_id);
		return;
	}

	RTE_LOG(INFO, USER1, "Entering main loop on lcore %u\n", lcore_id);

	for (i = 0; i < qconf->n_rx_port; i++) {
		portid = qconf->rx_port_list[i];
		qconf->start_rx_flag[i] = true;
		qconf->rx_burst_try[i] = 0;
	}

	/* Main loop for receiving, processing and sending packets
	 * Currently use synchronous and non-blocking IO functions
	 * */
	while (!force_quit) {

		cur_tsc = rte_rdtsc();

		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {
			/* TX burst queue drain */
			for (i = 0; i < qconf->n_rx_port; i++) {
				/* Get corresponded tx port of the rx port */
				portid = l2fwd_dst_ports[qconf->rx_port_list[i]];
				buffer = tx_buffer[portid];
				/* Drain all buffered packets in the tx queue */
				sent = rte_eth_tx_buffer_flush(portid, 0, buffer);
				if (sent) {
					RTE_LOG(DEBUG, USER1, "Drain %d UDP packets in the tx queue\n", sent);
				}
			}

			prev_tsc = cur_tsc;
		}

		/*
		 * Read packets from all RX ports
		 * TODO: Optimize it for n_rx_port == 1, no looping is needed there.
		 */
		for (i = 0; i < qconf->n_rx_port; i++) {
			portid = qconf->rx_port_list[i];
			/* MARK: A burst number of packets are returned by this
			 * function, so can not get the recv timestamps for each packet
			 * here. Only is the burst number is one.
			 * */
			if (qconf->start_rx_flag[i]) {
				nb_rx = rte_eth_rx_burst(portid, 0, pkts_burst, MAX_PKT_BURST);
				if (likely(nb_rx == 0)) {
					/* execute "pause" instruction to avoid context switch for
					 * short sleep */
					qconf->rx_burst_try[i] += 1;
					rte_delay_us(BURST_RX_TRY_US);
				}
				else {
					/* Process received burst packets */
					st_proc_tsc = rte_rdtsc();
					for (j = 0; j < nb_rx; j++) {
						m = pkts_burst[j];
						/* Filter out packets:
						 * - Non-UDP packets
						 * - Looping packets
						 * */
						/* TODO: Handle prefetches */
						if ( (filter_ret = filter_ether_frame(m)) != 1 ) {
							RTE_LOG(DEBUG, USER1, "TSC: %ld, The frame doesn't pass the filter. Error code: %d\n", rte_rdtsc(), filter_ret);
							rte_pktmbuf_free(m);
							continue;
						}
						else {
							nb_udp_dgrams += 1;
							if (appending_ts) {
								/* Append timestamp to a  UDP packet */
								udp_append_ts(m);
							}
							rte_prefetch0(rte_pktmbuf_mtod(m, void *));
							l2fwd_simple_forward(m, portid);
						}
					}
					/* Evaluate processing delay of burst packets */
					if (verbose == 1) {
						proc_tsc = rte_rdtsc() - st_proc_tsc;
						proc_time = (1.0 / rte_get_timer_hz()) * proc_tsc * 1000;
						RTE_LOG(INFO, USER1,
								"[Port:%d] Process a burst of %d packets, proc time: %.4f ms, number of already received UDP packets: %ld\n",
								portid, nb_rx, proc_time, nb_udp_dgrams);
					}
					/* Reset rx try times */
					qconf->rx_burst_try[i] = 0;
				}
				if (qconf->rx_burst_try[i] >= MAX_BURST_RX_TRY) {
					/* Long sleep force running thread to suspend */
					usleep(RX_LONG_SLEEP_US);
					/* TODO: Check the link status(relative costly) and update rx flags */
				}
			}
			else {
				/* TODO(Maybe): Check the link status and update start_rx flags */
			}

		}
	}
}

	static int
l2fwd_launch_one_lcore(__attribute__((unused)) void *dummy)
{
	l2fwd_main_loop();
	return 0;
}

/* display usage */
	static void
l2fwd_usage(const char *prgname)
{
	printf("%s [EAL options] -- -p PORTMASK [-q NQ]\n"
			"  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
			"  -q NQ: number of queue (=ports) per lcore (default is 1)\n"
			" -s MAC: Source MAC address presented in XX:XX:XX:XX:XX:XX format\n"
			" -d MAC: Destination MAC address presented in XX:XX:XX:XX:XX:XX format\n"
			"  --[no-]mac-updating: Enable or disable MAC addresses updating (enabled by default)\n"
			"      When enabled:\n"
			"       - The source MAC address is replaced by the TX port MAC address\n"
			"       - The destination MAC address is replaced by the MAC provided by -d option\n"
			"  --[no-]packet-capturing: Enable or disable packet capturing (disabled by default)\n"
			"      When enabled:\n"
			"       - The the pdump capture framework is intialized, the packets can be captured by official pdump-tool\n"
			"  --[no-]appending-ts: Enable appending timestamps at the end of UDP payload (enabled by default)\n"
			"      When disabled: The app just perform simple forwarding\n"
			"  --[no-]debugging: Enable or disable debugging mode (disabled by default)\n"
			"      When enabled:\n"
			"       - The logging level is set to DEBUG and additional debug variables are created. (May slow down the program)\n",
			prgname);
}

	static int
l2fwd_parse_portmask(const char *portmask)
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

	static unsigned int
l2fwd_parse_nqueue(const char *q_arg)
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

	__attribute__((unused)) static int
l2fwd_parse_timer_period(const char *q_arg)
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

static const char short_options[] =
"p:"  /* portmask */
"q:"  /* number of queues */
"s:"  /* source MAC address */
"d:" /* destination MAC address */
"v:" /* verbose level */
;

#define CMD_LINE_OPT_MAC_UPDATING "mac-updating"
#define CMD_LINE_OPT_NO_MAC_UPDATING "no-mac-updating"
#define CMD_LINE_OPT_PACKET_CAPTURING "packet-capturing"
#define CMD_LINE_OPT_NO_PACKET_CAPTURING "no-packet-capturing"
#define CMD_LINE_OPT_DEBUGGING "debugging"
#define CMD_LINE_OPT_NO_DEBUGGING "no-debugging"
#define CMD_LINE_OPT_APPENDING_TS "appending-ts"
#define CMD_LINE_OPT_NO_APPENDING_TS "no-appending-ts"

enum {
	/* long options mapped to a short option */

	/* first long only option value must be >= 256, so that we won't
	 * conflict with short options */
	CMD_LINE_OPT_MIN_NUM = 256,
};

static const struct option lgopts[] = {
	{ CMD_LINE_OPT_MAC_UPDATING, no_argument, &mac_updating, 1},
	{ CMD_LINE_OPT_NO_MAC_UPDATING, no_argument, &mac_updating, 0},
	{ CMD_LINE_OPT_PACKET_CAPTURING, no_argument, &packet_capturing, 1},
	{ CMD_LINE_OPT_NO_PACKET_CAPTURING, no_argument, &packet_capturing, 0},
	{ CMD_LINE_OPT_DEBUGGING, no_argument, &debugging, 1},
	{ CMD_LINE_OPT_NO_DEBUGGING, no_argument, &debugging, 0},
	{ CMD_LINE_OPT_APPENDING_TS, no_argument, &appending_ts, 1},
	{ CMD_LINE_OPT_NO_APPENDING_TS, no_argument, &appending_ts, 0},
	{NULL, 0, 0, 0}
};

/* Parse the argument given in the command line of the application */
	static int
l2fwd_parse_args(int argc, char **argv)
{
	int opt, ret, i;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];
	char *end;
	end = NULL;

	argvopt = argv;

	while ((opt = getopt_long(argc, argvopt, short_options,
					lgopts, &option_index)) != EOF) {

		switch (opt) {
			/* portmask */
			case 'p':
				l2fwd_enabled_port_mask = l2fwd_parse_portmask(optarg);
				if (l2fwd_enabled_port_mask == 0) {
					printf("Invalid portmask\n");
					l2fwd_usage(prgname);
					return -1;
				}
				break;

				/* nqueue */
			case 'q':
				l2fwd_rx_queue_per_lcore = l2fwd_parse_nqueue(optarg);
				if (l2fwd_rx_queue_per_lcore == 0) {
					printf("Invalid queue number\n");
					l2fwd_usage(prgname);
					return -1;
				}
				break;

			case 'd':
				sscanf(optarg, "%02x:%02x:%02x:%02x:%02x:%02x",
						&dst_mac[0], &dst_mac[1], &dst_mac[2], &dst_mac[3], &dst_mac[4], &dst_mac[5]);
				for (i = 0; i < ETHER_ADDR_LEN; ++i) {
					dst_mac_addr.addr_bytes[i] = (uint8_t) dst_mac[i];
				}
				break;

			case 's':
				sscanf(optarg, "%02x:%02x:%02x:%02x:%02x:%02x",
						&src_mac[0], &src_mac[1], &src_mac[2], &src_mac[3], &src_mac[4], &src_mac[5]);
				for (i = 0; i < ETHER_ADDR_LEN; ++i) {
					src_mac_addr.addr_bytes[i] = (uint8_t) src_mac[i];
				}
				break;

			case 'v':
				verbose = strtol(optarg, &end, 10);
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
		argv[optind-1] = prgname;

	ret = optind-1;
	optind = 1; /* reset getopt lib */
	return ret;
}

/* Check the link status of all ports in up to 9s, and print them finally */
	static void
check_all_ports_link_status(uint32_t port_mask)
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
		RTE_ETH_FOREACH_DEV(portid) {
			if (force_quit)
				return;
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf(
							"Port%d Link Up. Speed %u Mbps - %s\n",
							portid, link.link_speed,
							(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
							("full-duplex") : ("half-duplex\n"));
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

	static void
signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

	int
main(int argc, char **argv)
{
	struct lcore_queue_conf *qconf;
	int ret;
	uint16_t nb_ports;
	uint16_t nb_ports_available = 0;
	uint16_t portid, last_port;
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

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* parse application arguments (after the EAL ones) */
	ret = l2fwd_parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid L2FWD arguments\n");

	RTE_LOG(INFO, USER1, "DEBUG mode: %s\n", debugging? "enabled" : "disabled");
	if (debugging) {
		/* init logger */
		rte_log_set_global_level(RTE_LOG_DEBUG);
		rte_log_set_level(RTE_LOGTYPE_USER1 , RTE_LOG_DEBUG);
	}
	else {
		rte_log_set_global_level(RTE_LOG_INFO);
		rte_log_set_level(RTE_LOGTYPE_USER1 , RTE_LOG_INFO);
	}

	RTE_LOG(INFO, USER1, "Verbose level: %d\n", verbose);

	RTE_LOG(INFO, USER1, "MAC updating: %s\n", mac_updating ? "enabled" : "disabled");

	if (mac_updating) {
		RTE_LOG(INFO, USER1, "Source MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
				src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
		RTE_LOG(INFO, USER1, "Destination MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
				dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4], dst_mac[5]);
	}

	RTE_LOG(INFO, USER1, "Appending timestamps: %s\n", appending_ts? "enabled" : "disabled");

	RTE_LOG(INFO, USER1, "Packet capturing: %s\n", packet_capturing? "enabled" : "disabled");
	/* Initialise the pdump framework */
	if (packet_capturing) {
		/* Initialise the pdump framework */
		ret = rte_pdump_init(NULL);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Can not initialize the pdump framework.");
	}

	/* MARK: This function is not supported in DPDK 17.11 */
	/*nb_ports = rte_eth_dev_count_avail();*/
	nb_ports = 2;
	RTE_LOG(INFO, USER1, "Number of to be used ports: %d\n", nb_ports);
	/*if (nb_ports == 0)*/
	/*rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");*/

	/* check port mask to possible port mask */
	if (l2fwd_enabled_port_mask & ~((1 << nb_ports) - 1))
		rte_exit(EXIT_FAILURE, "Invalid portmask; possible (0x%x)\n",
				(1 << nb_ports) - 1);

	/* reset l2fwd_dst_ports */
	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
		l2fwd_dst_ports[portid] = 0;
	last_port = 0;

	/*
	 * TODO: Check core affinity
	 * Each logical core is assigned a dedicated TX queue on each port.
	 */
	RTE_ETH_FOREACH_DEV(portid) {
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;

		if (nb_ports_in_mask % 2) {
			l2fwd_dst_ports[portid] = last_port;
			l2fwd_dst_ports[last_port] = portid;
		}
		else
			last_port = portid;

		nb_ports_in_mask++;
	}
	if (nb_ports_in_mask % 2) {
		l2fwd_dst_ports[last_port] = last_port;
	}

	rx_lcore_id = 0;
	qconf = NULL;

	/* Initialize the port/queue configuration of each logical core */
	RTE_ETH_FOREACH_DEV(portid) {
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
		RTE_LOG(INFO, USER1, "Lcore %u: RX port %u\n", rx_lcore_id, portid);
	}

	nb_mbufs = RTE_MAX(nb_ports * (nb_rxd + nb_txd + MAX_PKT_BURST +
				nb_lcores * MEMPOOL_CACHE_SIZE), 8192U);

	/* create the mbuf pool */
	l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", nb_mbufs,
			MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
			rte_socket_id());
	if (l2fwd_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

	/*Initialise each port*/
	RTE_ETH_FOREACH_DEV(portid) {
		struct rte_eth_rxconf rxq_conf;
		struct rte_eth_txconf txq_conf;
		struct rte_eth_conf local_port_conf = port_conf;
		struct rte_eth_dev_info dev_info;

		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0) {
			RTE_LOG(INFO,USER1,"Skipping disabled port %u\n", portid);
			continue;
		}
		nb_ports_available++;

		/* init port */
		RTE_LOG(INFO, USER1, "Initializing port %u... \n", portid);
		fflush(stdout);
		rte_eth_dev_info_get(portid, &dev_info);
		if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
			local_port_conf.txmode.offloads |=
				DEV_TX_OFFLOAD_MBUF_FAST_FREE;
		ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
					ret, portid);

		ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,
				&nb_txd);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
					"Cannot adjust number of descriptors: err=%d, port=%u\n",
					ret, portid);

		/* [Zuo] Get MAC addresses of all enabled port */
		rte_eth_macaddr_get(portid, &l2fwd_ports_eth_addr[portid]);

		/* init one RX queue */
		fflush(stdout);
		rxq_conf = dev_info.default_rxconf;
		rxq_conf.offloads = local_port_conf.rxmode.offloads;
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
				rte_eth_dev_socket_id(portid),
				&rxq_conf,
				l2fwd_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
					ret, portid);

		/* init one TX queue on each port */
		fflush(stdout);
		txq_conf = dev_info.default_txconf;
		txq_conf.txq_flags = ETH_TXQ_FLAGS_IGNORE;
		txq_conf.offloads = local_port_conf.txmode.offloads;
		ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
				rte_eth_dev_socket_id(portid),
				&txq_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
					ret, portid);

		/* Initialize TX buffers */
		tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
				RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
				rte_eth_dev_socket_id(portid));
		if (tx_buffer[portid] == NULL)
			rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
					portid);

		rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

		ret = rte_eth_tx_buffer_set_err_callback(tx_buffer[portid],
				rte_eth_tx_buffer_count_callback,
				&port_statistics[portid].dropped);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
					"Cannot set error callback for tx buffer on port %u\n",
					portid);

		/* Start device */
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
					ret, portid);
		RTE_LOG(INFO, USER1, "Device started for port: %d\n", portid);

		/* MARK: Enable promiscuous mode in each enabled port */
		rte_eth_promiscuous_enable(portid);

		RTE_LOG(INFO, USER1, "Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
				portid,
				l2fwd_ports_eth_addr[portid].addr_bytes[0],
				l2fwd_ports_eth_addr[portid].addr_bytes[1],
				l2fwd_ports_eth_addr[portid].addr_bytes[2],
				l2fwd_ports_eth_addr[portid].addr_bytes[3],
				l2fwd_ports_eth_addr[portid].addr_bytes[4],
				l2fwd_ports_eth_addr[portid].addr_bytes[5]);

		/* initialize port stats */
		memset(&port_statistics, 0, sizeof(port_statistics));
	}

	if (!nb_ports_available) {
		rte_exit(EXIT_FAILURE,
				"All available ports are disabled. Please set portmask.\n");
	}

	check_all_ports_link_status(l2fwd_enabled_port_mask);

	ret = 0;
	/* launch per-lcore init on every lcore */
	rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, NULL, CALL_MASTER);

	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}

	/* Cleanups */
	RTE_LOG(INFO, EAL, "Run cleanups.\n");
	RTE_ETH_FOREACH_DEV(portid) {
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		RTE_LOG(INFO,USER1,"Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		RTE_LOG(INFO,USER1," Done\n");
	}

	if (packet_capturing)
		rte_pdump_uninit();

	RTE_LOG(INFO, EAL, "App exits.\n");
	/* MARK: In DPDK 17.11-rc4, rte_eal_cleanup is depreciated, can not be used
	 *		 So exiting the program results in leaking hugepages.
	 *		 rte_eal_cleanup feature is provided by DPDK 18.02
	 * */
	/*RTE_LOG(INFO, EAL, "Release hugepages.\n");*/
	/*rte_eal_cleanup();*/

	return ret;
}