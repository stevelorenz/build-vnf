/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *    Description:  Network coding UDP flows with NCKernel
 *
 *                  - With default mode, only UDP payload is coded.
 *
 *      Dev-Note:
 *                  - Currently, no call-backs are used. The ethernet frames are
 *                    handled one by one:
 *                      Recv_RX_Queue -> Filter -> Code -> Rewrite_MAC ->
 *                      Put_TX_Queue -> Drain_TX_Queue
 *
 *                  - For all coder types, a coding buffer(static uint8_t array)
 *                    is used to exchange data between mbufs and coder's own
 *                    buffer. The input mbuf is always freed after the coding
 *                    operaton of all coder types. Addtional mbufs are cloned to
 *                    encapsulate output(s) of the coder.
 *
 *        Version:  0.3
 *          Email:  xianglinks@gmail.com
 *
 * =====================================================================================
 */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_interrupts.h>
#include <rte_ip.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_pdump.h>
#include <rte_per_lcore.h>
#include <rte_prefetch.h>
#include <rte_random.h>
#include <rte_udp.h>

#include <nckernel.h>
#include <skb.h>

typedef void (*UDP_NC_FUNC)(struct rte_mbuf*, uint16_t);

/* Maximal length of the coding buffer */
#define MAX_NC_BUFFER 1400

#define MAGIC_NUM 117

#define TEST_PORT_ID 821

#ifndef DEBUG
#define DEBUG 0
#endif /* ifndef DEBUG 0 */

#ifndef PROTOCOL
#define PROTOCOL "noack"
#endif

#ifndef SYMBOL_SIZE
#define SYMBOL_SIZE "258"
#endif

#ifndef SYMBOLS
#define SYMBOLS "2"
#endif

#ifndef REDUNDANCY
#define REDUNDANCY "1"
#endif

/**********************
 *  Global Variables  *
 **********************/

static volatile bool force_quit;

/* Enable debug mode */
static int debugging = 0;

/* MAC updating enabled by default */
static int mac_updating = 1;

/* Disable packet capture by default
 * Packet capture is supported by pdump library
 * */
static int packet_capturing = 0;

/* Flag for processing operation of UDP segments */
static int coder_type = 0;

/* Number of to be used ports */
static int nb_ports = 0;

/* Burst-based packet processing */
/* Default maximal received packets each time
 * Smaller pkt_burst -> lower latency -> lower bandwidth
 * */
static int max_pkt_burst = 32;
/* Make these parameters tunable */
static int poll_short_interval_us; /* Try to receive from the RX every X
                                      microseconds */
static int max_poll_short_try;
static int poll_long_interval_us;

#define BURST_TX_DRAIN_US 10
static int burst_tx_drain_us = 10;
#define MEMPOOL_CACHE_SIZE 256

/* Processing delay of frames */
static uint64_t st_proc_tsc = 0;
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
        volatile bool
            start_rx_flag[MAX_RX_QUEUE_PER_LCORE]; /* A flag indicate the
                                                      queue should start
                                                      burst receiving */
        uint16_t rx_burst_try[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;

struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

static struct rte_eth_dev_tx_buffer* tx_buffer[RTE_MAX_ETHPORTS];

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
struct rte_mempool* l2fwd_pktmbuf_pool = NULL;

/* NC coders and operation functions */
struct nck_encoder enc;
struct nck_decoder dec;
struct nck_recoder rec;
struct rte_mbuf* m_cmy;
static void (*udp_nc_opt)(struct rte_mbuf*, uint16_t);

/****************
 *  Prototypes  *
 ****************/

UDP_NC_FUNC get_nc_func(int coder_type);
static void l2_forward_rxqueue(struct rte_mbuf* m, unsigned portid);
static void udp_encode(struct rte_mbuf* m_in, uint16_t portid);
static void udp_recode(struct rte_mbuf* m_in, uint16_t portid);
static void udp_decode(struct rte_mbuf* m_in, uint16_t portid);

/******************************
 *  Function Implementations  *
 ******************************/

/**
 * @brief Convert Uint32 IP address to x.x.x.x format
 */
__attribute__((unused)) static void get_ip_str(uint32_t ip, char* ip_addr_str)
{
        ip = ntohl(ip);
        uint8_t octet[4];
        /*char ip_addr_str[16];*/
        for (int i = 0; i < 4; i++) {
                octet[i] = ip >> (i * 8);
        }
        sprintf(
            ip_addr_str, "%d.%d.%d.%d", octet[3], octet[2], octet[1], octet[0]);
}

/**
 * @brief Filter a Ethernet frame, return true if frame match the rules.
 */
__attribute__((unused)) static int8_t filter_ether_frame(struct rte_mbuf* m)
{
        struct ether_hdr* ethh;
        ethh = rte_pktmbuf_mtod(m, struct ether_hdr*);
        /* Filter out non-IP packets */
        if (ethh->ether_type != rte_cpu_to_be_16(ETHER_TYPE_IPv4))
                return -1;

        /* Filter out non-UDP packets */
        struct ipv4_hdr* iph;
        iph = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr*, ETHER_HDR_LEN);
        if (iph->next_proto_id != IPPROTO_UDP)
                return -2;

        /* Filter out looping packets with source MAC address of any forwarding
         * ports  */
        uint8_t i;
        for (i = 0; i < nb_ports; ++i) {
                if (is_same_ether_addr(
                        &(ethh->s_addr), &(l2fwd_ports_eth_addr[i])))
                        return -3;
        }

        return 1;
}

__attribute__((unused)) static bool is_ipv4_pkt(struct rte_mbuf* m)
{
        struct ether_hdr* ethh;
        ethh = rte_pktmbuf_mtod(m, struct ether_hdr*);
        if (ethh->ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4)) {
                return true;
        }
        return false;
}

__attribute__((unused)) static bool is_udp_dgram(struct rte_mbuf* m)
{
        struct ipv4_hdr* iph;
        iph = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr*, ETHER_HDR_LEN);
        if (iph->next_proto_id == IPPROTO_UDP) {
                return true;
        }

        return false;
}

static void recalc_cksum(struct ipv4_hdr* iph, struct udp_hdr* udph)
{
        udph->dgram_cksum = 0;
        iph->hdr_checksum = 0;
        udph->dgram_cksum = rte_ipv4_udptcp_cksum(iph, udph);
        iph->hdr_checksum = rte_ipv4_cksum(iph);
}

/**
 * @brief udp_encode: Put the UDP payload of incoming mbuf into encoder and put
 * all outputs of the encoder to the TX queue identified by portid.
 *
 * @note:
 *
 * - If only coded the UDP payload, the length is assumed to be the same for all
 *   coming segments. Otherwise the length (UDP total length) should also be
 *   coded. Since potential padding operations could be performed during
 *   encoding and the length information is lost.
 *
 * - Since En(De)coder has its own memory buffer to store data, no shared
 *   temporary buffer is needed.
 *
 */
static void udp_encode(struct rte_mbuf* m_in, uint16_t portid)
{
        static struct sk_buff skb;
        uint16_t in_pl_len = 0; // length of input UDP payload
        uint16_t num_coded = 0;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        struct rte_mbuf* m_out;
        uint8_t* mbuf_data;
        /* A encoding buffer is used since the length of the encoder output is
         * unknown before calling nck_get_coded */
        static uint8_t enc_buf[MAX_NC_BUFFER];

        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph
            + ((iph->version_ihl & 0x0F) * 32 / 8));
        /* MARK: This one is calculated wegen that currently I do not know how
         * to get the pointer to the end of the UDP payload */
        in_pl_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;
        mbuf_data = (uint8_t*)(udph) + UDP_HDR_LEN;

        skb_new(&skb, enc_buf, sizeof(enc_buf));
        skb_reserve(&skb, sizeof(uint16_t)); // reserve only empty skbs
        rte_memcpy(skb.data, mbuf_data, in_pl_len);
        skb_put(&skb, in_pl_len);
        skb_push_u16(&skb, skb.len); // htons
        nck_put_source(&enc, &skb);

        /*if (nck_put_source(&enc, &skb) == -1) {*/
        /*RTE_LOG(ERR, "[ENC] Invalid input for encoder.");*/
        /*rte_pktmbuf_free(m_in);*/
        /*return;*/
        /*}*/

        while (nck_has_coded(&enc)) {
                /* Clone a new mbuf for output */
                num_coded += 1;
                m_out = rte_pktmbuf_clone(m_in, l2fwd_pktmbuf_pool);
                iph = rte_pktmbuf_mtod_offset(
                    m_out, struct ipv4_hdr*, ETHER_HDR_LEN);
                udph = (struct udp_hdr*)((char*)iph
                    + ((iph->version_ihl & 0x0F) * 32 / 8));
                mbuf_data = (uint8_t*)(udph) + UDP_HDR_LEN;

                skb_new(&skb, enc_buf, sizeof(enc_buf));
                skb_reserve(&skb, sizeof(uint8_t));
                nck_get_coded(&enc, &skb);
                skb_push_u8(&skb, MAGIC_NUM);
                RTE_LOG(DEBUG, USER1,
                    "[ENC] Output num: %u, Input length: %u, Encoded data "
                    "length: %u.\n",
                    num_coded, in_pl_len, skb.len);
                rte_pktmbuf_append(m_out, (skb.len - in_pl_len));
                rte_memcpy(mbuf_data, skb.data, skb.len);

                udph->dgram_len = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                iph->total_length
                    = rte_cpu_to_be_16(rte_be_to_cpu_16(iph->total_length)
                        + (skb.len - in_pl_len));
                recalc_cksum(iph, udph);
                if (unlikely(coder_type == -1)) {
                        m_cmy = rte_pktmbuf_clone(m_in, l2fwd_pktmbuf_pool);
                        udp_recode(m_out, portid);
                } else {
                        l2_forward_rxqueue(m_out, portid);
                }
        }

        rte_pktmbuf_free(m_in);
}

static void udp_recode(struct rte_mbuf* m_in, uint16_t portid)
{
        static struct sk_buff skb;
        static uint8_t rec_buf[MAX_NC_BUFFER];
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint16_t in_pl_len;
        uint16_t num_recoded = 0;
        struct rte_mbuf* m_out = NULL;
        uint8_t* mbuf_data;

        skb_new(&skb, rec_buf, sizeof(rec_buf));
        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph
            + ((iph->version_ihl & 0x0F) * 32 / 8));
        in_pl_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;
        mbuf_data = (uint8_t*)(udph) + UDP_HDR_LEN;
        rte_memcpy(skb.data, mbuf_data, in_pl_len);
        skb_put(&skb, in_pl_len);
        if (skb_pull_u8(&skb) != MAGIC_NUM) {
                rte_pktmbuf_free(m_in);
                RTE_LOG(DEBUG, USER1, "[REC] Recv an invalid input.\n");
                return;
        }
        nck_put_coded(&rec, &skb);

        while (nck_has_coded(&rec)) {
                /* Clone a new mbuf for output */
                num_recoded += 1;
                m_out = rte_pktmbuf_clone(m_in, l2fwd_pktmbuf_pool);
                iph = rte_pktmbuf_mtod_offset(
                    m_out, struct ipv4_hdr*, ETHER_HDR_LEN);
                udph = (struct udp_hdr*)((char*)iph
                    + ((iph->version_ihl & 0x0F) * 32 / 8));
                mbuf_data = (uint8_t*)(udph) + UDP_HDR_LEN;

                skb_new(&skb, rec_buf, sizeof(rec_buf));
                skb_reserve(&skb, sizeof(uint8_t));
                nck_get_coded(&rec, &skb);
                skb_push_u8(&skb, MAGIC_NUM);
                RTE_LOG(DEBUG, USER1,
                    "[REC] Output num: %u, Input length: %u, Recoded data "
                    "length: %u.\n",
                    num_recoded, in_pl_len, skb.len);
                rte_pktmbuf_append(m_out, (skb.len - in_pl_len));
                rte_memcpy(mbuf_data, skb.data, skb.len);

                udph->dgram_len = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                iph->total_length
                    = rte_cpu_to_be_16(rte_be_to_cpu_16(iph->total_length)
                        + (skb.len - in_pl_len));
                recalc_cksum(iph, udph);

                if (unlikely(coder_type == -1)) {
                        udp_decode(m_out, portid);
                } else {
                        l2_forward_rxqueue(m_out, portid);
                }
        }
        rte_pktmbuf_free(m_in);
}

/**
 * @brief udp_decode: Put the payload of incoming mbuf into decoder and put all
 * outputs of the decoder to the TX queue
 *
 * @note:
 */
static void udp_decode(struct rte_mbuf* m_in, uint16_t portid)
{
        static struct sk_buff skb;
        static uint8_t dec_buf[MAX_NC_BUFFER]; // Could be avoided
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint16_t in_pl_len;
        uint16_t num_decoded = 0;
        struct rte_mbuf* m_out = NULL;
        uint8_t* mbuf_data;

        skb_new(&skb, dec_buf, sizeof(dec_buf));
        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph
            + ((iph->version_ihl & 0x0F) * 32 / 8));
        in_pl_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;
        mbuf_data = (uint8_t*)(udph) + UDP_HDR_LEN;
        rte_memcpy(skb.data, mbuf_data, in_pl_len);
        skb_put(&skb, in_pl_len);
        if (skb_pull_u8(&skb) != MAGIC_NUM) {
                rte_pktmbuf_free(m_in);
                RTE_LOG(DEBUG, USER1, "[DEC] Recv an invalid input.\n");
                return;
        }
        nck_put_coded(&dec, &skb);

        if (!nck_has_source(&dec)) {
                rte_pktmbuf_free(m_in);
                RTE_LOG(DEBUG, USER1, "[DEC] Decoder has no output.\n");
                return;
        }

        while (nck_has_source(&dec)) {
                num_decoded += 1;
                m_out = rte_pktmbuf_clone(m_in, l2fwd_pktmbuf_pool);
                iph = rte_pktmbuf_mtod_offset(
                    m_out, struct ipv4_hdr*, ETHER_HDR_LEN);
                udph = (struct udp_hdr*)((char*)iph
                    + ((iph->version_ihl & 0x0F) * 32 / 8));
                mbuf_data = (uint8_t*)(udph) + UDP_HDR_LEN;
                skb_new(&skb, dec_buf, in_pl_len);
                nck_get_source(&dec, &skb);
                skb.len = skb_pull_u16(&skb);
                rte_pktmbuf_trim(m_out, (in_pl_len - skb.len));
                rte_memcpy((char*)(udph) + UDP_HDR_LEN, skb.data, skb.len);
                RTE_LOG(DEBUG, USER1,
                    "[DEC] Output num: %u, Input length: %u, Decoded "
                    "data length: %u.\n",
                    num_decoded, in_pl_len, skb.len);

                udph->dgram_len = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                iph->total_length
                    = rte_cpu_to_be_16(rte_be_to_cpu_16(iph->total_length)
                        - (in_pl_len - skb.len));
                recalc_cksum(iph, udph);

                if (unlikely(coder_type == -1)) {
                        /* The m_out and m_cmy should be the same */
                        RTE_ASSERT(memcmp(m_out, m_cmy, m_out.buf_len) == 0);
                        rte_pktmbuf_free(m_cmy);
                } else {
                        l2_forward_rxqueue(m_out, portid);
                }
        }

        rte_pktmbuf_free(m_in);
}

UDP_NC_FUNC get_nc_func(int coder_type)
{
        if (coder_type == 0) {
                return udp_encode;
        }
        if (coder_type == 1) {
                return udp_decode;
        }
        if (coder_type == -1) {
                return udp_encode;
        }

        RTE_LOG(INFO, USER1, "[WARN] Just forwarding\n");
        return NULL;
}

inline static void l2fwd_mac_updating(struct rte_mbuf* m)
{
        struct ether_hdr* eth;
        eth = rte_pktmbuf_mtod(m, struct ether_hdr*);
        /* Update destination and source MAC addr */
        ether_addr_copy(&src_mac_addr, &eth->s_addr);
        ether_addr_copy(&dst_mac_addr, &eth->d_addr);
}

static void l2_forward_rxqueue(struct rte_mbuf* m, unsigned portid)
{
        unsigned dst_port;
        int sent;
        struct rte_eth_dev_tx_buffer* buffer;

        dst_port = l2fwd_dst_ports[portid];

        if (mac_updating) {
                l2fwd_mac_updating(m);
        }

        buffer = tx_buffer[dst_port];

        sent = rte_eth_tx_buffer(dst_port, 0, buffer, m);

        /* Possible draining processes */
        if (sent) {
                RTE_LOG(DEBUG, USER1,
                    "Trigger %d UDP packets drained in the TX buffer\n", sent);
        }
}

/* Main forwarding and processing loop
 *
 * MARK: This function handles packet receiving, processing and transmitting.
 * */
static void l2fwd_main_loop(void)
{
        /* MARK: Packets are read in a burst of size MAX_PKT_BURST
         * This is also a tunable parameter for latency
         * */
        struct rte_mbuf* pkts_burst[max_pkt_burst];
        struct rte_mbuf* m;
        unsigned lcore_id;
        unsigned i, j, portid, nb_rx;
        int8_t filter_ret = 0;
        struct lcore_queue_conf* qconf;
        uint64_t prev_tsc, diff_tsc, cur_tsc;
        uint16_t sent;

        /* MARK: Why there is a US_PER_S - 1 is added? Maybe check jiffies */
        const uint64_t drain_tsc
            = (rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * burst_tx_drain_us;
        struct rte_eth_dev_tx_buffer* buffer;

        prev_tsc = 0;

        lcore_id = rte_lcore_id();
        qconf = &lcore_queue_conf[lcore_id]; /* queue configuration */

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
         * TODO: Refactor the code here
         * */
        while (!force_quit) {

                cur_tsc = rte_rdtsc();

                diff_tsc = cur_tsc - prev_tsc;
                if (unlikely(diff_tsc > drain_tsc)) {
                        /* TX burst queue drain */
                        for (i = 0; i < qconf->n_rx_port; i++) {
                                /* Get corresponded tx port of the rx port */
                                portid
                                    = l2fwd_dst_ports[qconf->rx_port_list[i]];
                                buffer = tx_buffer[portid];
                                /* Drain all buffered packets in the tx queue */
                                sent = rte_eth_tx_buffer_flush(
                                    portid, 0, buffer);
                                if (sent) {
                                        RTE_LOG(DEBUG, USER1,
                                            "Drain %d UDP packets in the tx "
                                            "queue\n",
                                            sent);
                                }
                        }

                        prev_tsc = cur_tsc;
                }

                /* TODO:  <22-07-18,zuo>
                 * - The filtering can be peformed on multiple mbufs instead of
                 *   one by one. (Reduce number of filter function calls)
                 * */
                for (i = 0; i < qconf->n_rx_port; i++) {
                        portid = qconf->rx_port_list[i];
                        /* MARK: A burst number of packets are returned by this
                         * function, so can not get the recv timestamps for each
                         * packet here. Only is the burst number is one.
                         * */
                        if (qconf->start_rx_flag[i]) {
                                nb_rx = rte_eth_rx_burst(
                                    portid, 0, pkts_burst, max_pkt_burst);
                                if (likely(nb_rx == 0)) {
                                        if (max_poll_short_try == 0) {
                                                /* Keep polling, SHOULD use 100%
                                                 * of CPU */
                                                continue;
                                        }
                                        /* execute "pause" instruction to avoid
                                         * context switch for short sleep */
                                        if (qconf->rx_burst_try[i]
                                            < max_poll_short_try) {
                                                qconf->rx_burst_try[i] += 1;
                                                rte_delay_us(
                                                    poll_short_interval_us);
                                        } else {
                                                /* Long sleep force running
                                                 * thread to suspend */
                                                usleep(poll_long_interval_us);
                                        }
                                } else {
                                        /* Process received burst packets */
                                        /* Handle prefetches */
                                        st_proc_tsc = rte_rdtsc();
                                        for (j = 0; j < nb_rx; j++) {
                                                m = pkts_burst[j];
                                                rte_prefetch0(
                                                    rte_pktmbuf_mtod(m, void*));
                                                if ((filter_ret
                                                        = filter_ether_frame(m))
                                                    != 1) {
                                                        RTE_LOG(DEBUG, USER1,
                                                            "TSC: %ld, The "
                                                            "frame doesn't "
                                                            "pass the filter. "
                                                            "Error code: %d\n",
                                                            rte_rdtsc(),
                                                            filter_ret);
                                                        rte_pktmbuf_free(m);
                                                        continue;
                                                }
                                                nb_udp_dgrams += 1;
                                                rte_prefetch0(
                                                    rte_pktmbuf_mtod(m, void*));
                                                udp_nc_opt(m, portid);
                                        }
                                        /* Evaluate processing delay of burst
                                         * packets */
                                        if (debugging == 1) {
                                                proc_tsc
                                                    = rte_rdtsc() - st_proc_tsc;
                                                proc_time
                                                    = (1.0 / rte_get_timer_hz())
                                                    * proc_tsc * 1000;
                                                RTE_LOG(INFO, USER1,
                                                    "[Port:%d] Process a burst "
                                                    "of %d packets, proc time: "
                                                    "%.4f ms, number of "
                                                    "already received UDP "
                                                    "packets: %ld\n",
                                                    portid, nb_rx, proc_time,
                                                    nb_udp_dgrams);
                                        }
                                        /* Reset rx try times */
                                        qconf->rx_burst_try[i] = 0;
                                }
                        } else {
                                /* TODO (Maybe): Check the link status and
                                 * update start_rx flags */
                        }
                }
        }
}

static int l2fwd_launch_one_lcore(__attribute__((unused)) void* dummy)
{
        l2fwd_main_loop();
        return 0;
}

/* display usage */
static void l2fwd_usage(const char* prgname)
{
        printf(
            "%s [EAL options] -- [APP options]\n"
            "-o CODERTYPE: NC coder type. 0->encoder, 1->decoder, 2->recoder.\n"
            "-p PORTMASK: hexadecimal bitmask of ports to configure\n"
            "-q NQ: number of queue (=ports) per lcore (default is 1)\n"
            "-n NP: number of to be used ports\n"
            "-s MAC: Source MAC address presented in XX:XX:XX:XX:XX:XX format\n"
            "-d MAC: Destination MAC address presented in XX:XX:XX:XX:XX:XX "
            "format\n"
            "-i "
            "max_poll_short_try,poll_short_interval_us,poll_long_interval_us\n"
            "	Comma split numbers for rx polling try number and intervals(in "
            "microseconds).\n"
            "   For example it can be 10,10,1000:"
            "--[no-]mac-updating: Enable or disable MAC addresses updating "
            "(enabled by default)\n"
            "	When enabled:\n"
            "       - The source MAC address is replaced by the TX port MAC "
            "address\n"
            "       - The destination MAC address is replaced by the MAC "
            "provided by -d option\n"
            "--[no-]packet-capturing: Enable or disable packet capturing "
            "(disabled by default)\n"
            "   When enabled:\n"
            "		- The the pdump capture framework is initialized, the "
            "packets can be captured by official pdump-tool\n"
            "--[no-]debugging: Enable or disable debugging mode (disabled by "
            "default)\n"
            "	When enabled:\n"
            "		- The logging level is set to DEBUG and additional "
            "debug variables are created. (May slow down the program)\n",
            prgname);
}

static int l2fwd_parse_portmask(const char* portmask)
{
        char* end = NULL;
        unsigned long pm;

        /* parse hexadecimal string */
        pm = strtoul(portmask, &end, 16);
        if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
                return -1;

        if (pm == 0)
                return -1;

        return pm;
}

static unsigned int l2fwd_parse_nqueue(const char* q_arg)
{
        char* end = NULL;
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

static const char short_options[]
    = "p:" /* portmask */
      "q:" /* number of queues */
      "s:" /* source MAC address */
      "d:" /* destination MAC address */
      "i:" /* rx polling parameters */
      "o:" /* UDP processing operation flag */
      "b:" /* maximal number of burst packets */
      "t:" /* period for draining the tx queue in us */
      "n:" /* number of ports */
    ;

#define CMD_LINE_OPT_MAC_UPDATING "mac-updating"
#define CMD_LINE_OPT_NO_MAC_UPDATING "no-mac-updating"
#define CMD_LINE_OPT_PACKET_CAPTURING "packet-capturing"
#define CMD_LINE_OPT_NO_PACKET_CAPTURING "no-packet-capturing"
#define CMD_LINE_OPT_DEBUGGING "debugging"
#define CMD_LINE_OPT_NO_DEBUGGING "no-debugging"

enum {
        /* long options mapped to a short option */

        /* first long only option value must be >= 256, so that we won't
         * conflict with short options */
        CMD_LINE_OPT_MIN_NUM = 256,
};

static const struct option lgopts[] = { { CMD_LINE_OPT_MAC_UPDATING,
                                            no_argument, &mac_updating, 1 },
        { CMD_LINE_OPT_NO_MAC_UPDATING, no_argument, &mac_updating, 0 },
        { CMD_LINE_OPT_PACKET_CAPTURING, no_argument, &packet_capturing, 1 },
        { CMD_LINE_OPT_NO_PACKET_CAPTURING, no_argument, &packet_capturing, 0 },
        { CMD_LINE_OPT_DEBUGGING, no_argument, &debugging, 1 },
        { CMD_LINE_OPT_NO_DEBUGGING, no_argument, &debugging, 0 },
        { NULL, 0, 0, 0 } };

/* Parse the argument given in the command line of the application */
static int l2fwd_parse_args(int argc, char** argv)
{
        int opt, ret, i;
        char** argvopt;
        int option_index;
        char* prgname = argv[0];
        char* end;
        end = NULL;

        argvopt = argv;

        while ((opt = getopt_long(
                    argc, argvopt, short_options, lgopts, &option_index))
            != EOF) {

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
                            &dst_mac[0], &dst_mac[1], &dst_mac[2], &dst_mac[3],
                            &dst_mac[4], &dst_mac[5]);
                        for (i = 0; i < ETHER_ADDR_LEN; ++i) {
                                dst_mac_addr.addr_bytes[i]
                                    = (uint8_t)dst_mac[i];
                        }
                        break;

                case 's':
                        sscanf(optarg, "%02x:%02x:%02x:%02x:%02x:%02x",
                            &src_mac[0], &src_mac[1], &src_mac[2], &src_mac[3],
                            &src_mac[4], &src_mac[5]);
                        for (i = 0; i < ETHER_ADDR_LEN; ++i) {
                                src_mac_addr.addr_bytes[i]
                                    = (uint8_t)src_mac[i];
                        }
                        break;

                case 'i':
                        sscanf(optarg, "%d,%d,%d", &max_poll_short_try,
                            &poll_short_interval_us, &poll_long_interval_us);
                        break;

                case 'o':
                        coder_type = strtol(optarg, &end, 10);
                        break;

                case 'b':
                        max_pkt_burst = strtol(optarg, &end, 10);
                        break;

                case 't':
                        burst_tx_drain_us = strtol(optarg, &end, 10);
                        break;

                case 'n':
                        nb_ports = strtoul(optarg, &end, 10);
                        if (nb_ports == 0) {
                                printf("Invalid port number!\n");
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
#define MAX_CHECK_TIME 90  /* 9s (90 * 100ms) in total */
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
                                        printf("Port%d Link Up. Speed %u Mbps "
                                               "- %s\n",
                                            portid, link.link_speed,
                                            (link.link_duplex
                                                == ETH_LINK_FULL_DUPLEX)
                                                ? ("full-duplex")
                                                : ("half-duplex\n"));
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
                printf(
                    "\n\nSignal %d received, preparing to exit...\n", signum);
                force_quit = true;
        }
}

/* TODO:  <26-07-18, zuo>
 * - Init mbufs properly to support NC directly on mbufs
 * - Check configs for vhost-user interfaces
 * */
int main(int argc, char** argv)
{
        struct lcore_queue_conf* qconf;
        int ret;
        uint16_t portid, last_port;
        unsigned lcore_id, rx_lcore_id;
        unsigned nb_ports_in_mask = 0;
        unsigned int nb_lcores = 0;
        unsigned int nb_mbufs;

        ret = rte_eal_init(argc, argv);
        if (ret < 0) {
                rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
        }
        argc -= ret;
        argv += ret;

        force_quit = false;
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        /* parse application arguments (after the EAL ones) */
        ret = l2fwd_parse_args(argc, argv);
        if (ret < 0)
                rte_exit(EXIT_FAILURE, "Invalid L2FWD arguments\n");

        RTE_LOG(INFO, USER1, "DEBUG mode: %s\n",
            debugging ? "enabled" : "disabled");
        if (debugging) {
                /* init logger */
                rte_log_set_global_level(RTE_LOG_DEBUG);
                rte_log_set_level(RTE_LOGTYPE_USER1, RTE_LOG_DEBUG);
        } else {
                rte_log_set_global_level(RTE_LOG_INFO);
                rte_log_set_level(RTE_LOGTYPE_USER1, RTE_LOG_INFO);
        }

        RTE_LOG(INFO, USER1, "MAC updating: %s\n",
            mac_updating ? "enabled" : "disabled");

        if (mac_updating) {
                RTE_LOG(INFO, USER1,
                    "Source MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4],
                    src_mac[5]);
                RTE_LOG(INFO, USER1,
                    "Destination MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    dst_mac[0], dst_mac[1], dst_mac[2], dst_mac[3], dst_mac[4],
                    dst_mac[5]);
        }

        RTE_LOG(INFO, USER1, "Packet capturing: %s\n",
            packet_capturing ? "enabled" : "disabled");
        /* Initialise the pdump framework */
        if (packet_capturing) {
                /* Initialise the pdump framework */
                ret = rte_pdump_init(NULL);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE,
                            "Can not initialize the pdump framework.");
        }

        RTE_LOG(INFO, USER1,
            "RX polling parameters: max_poll_short_try:%d, "
            "poll_short_interval_us:%d, poll_long_interval_us:%d\n",
            max_poll_short_try, poll_short_interval_us, poll_long_interval_us);

        RTE_LOG(INFO, USER1, "Number of to be used ports: %d\n", nb_ports);

        RTE_LOG(INFO, USER1,
            "Protocol: %s, Symbol size: %s, Symbols: %s, Redundancy:%s\n",
            PROTOCOL, SYMBOL_SIZE, SYMBOLS, REDUNDANCY);
        struct nck_option_value options[] = { { "protocol", PROTOCOL },
                { "symbol_size", SYMBOL_SIZE }, { "symbols", SYMBOLS },
                { "redundancy", REDUNDANCY }, { NULL, NULL } };

        switch (coder_type) {
        case 0:
                RTE_LOG(INFO, USER1, "Coder type: NC encoder.\n");

                if (nck_create_encoder(
                        &enc, NULL, options, nck_option_from_array)) {
                        rte_exit(EXIT_FAILURE, "Failed to create encoder.\n");
                }

                break;
        case 1:
                RTE_LOG(INFO, USER1, "Coder type: NC decoder.\n");
                if (nck_create_decoder(
                        &dec, NULL, options, nck_option_from_array)) {
                        rte_exit(EXIT_FAILURE, "Failed to create decoder.\n");
                }
                break;
        case 2:
                RTE_LOG(INFO, USER1, "Coder type: NC recoder.\n");
                if (nck_create_recoder(
                        &rec, NULL, options, nck_option_from_array)) {
                        rte_exit(EXIT_FAILURE, "Failed to create recoder.\n");
                }
                break;

        case -1:
                RTE_LOG(INFO, USER1,
                    "Coder type: NC encoder + recoder + decoder.\n");
                RTE_LOG(INFO, USER1, "[WARN] Used for debugging.\n");
                if (nck_create_encoder(
                        &enc, NULL, options, nck_option_from_array)) {
                        rte_exit(EXIT_FAILURE, "Failed to create encoder.\n");
                }
                if (nck_create_recoder(
                        &rec, NULL, options, nck_option_from_array)) {
                        rte_exit(EXIT_FAILURE, "Failed to create recoder.\n");
                }
                if (nck_create_decoder(
                        &dec, NULL, options, nck_option_from_array)) {
                        rte_exit(EXIT_FAILURE, "Failed to create decoder.\n");
                }
                break;

        case -2:
                RTE_LOG(INFO, USER1, "Coder type: Forwarding\n");
                break;

        default:
                rte_exit(EXIT_FAILURE, "Unknown coder type.\n");
        }

        udp_nc_opt = get_nc_func(coder_type);

        RTE_LOG(INFO, USER1, "Maximal number of burst packets: %d\n",
            max_pkt_burst);

        RTE_LOG(
            INFO, USER1, "Drain tx queue period: %d us\n", burst_tx_drain_us);

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
                l2fwd_dst_ports[last_port] = last_port;
        }

        rx_lcore_id = 0;
        qconf = NULL;

        /* Initialize the port/queue configuration of each logical core */
        RTE_ETH_FOREACH_DEV(portid)
        {
                /* skip ports that are not enabled */
                if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
                        continue;

                /* get the lcore_id for this port */
                while (rte_lcore_is_enabled(rx_lcore_id) == 0
                    || lcore_queue_conf[rx_lcore_id].n_rx_port
                        == l2fwd_rx_queue_per_lcore) {
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
                RTE_LOG(
                    INFO, USER1, "Lcore %u: RX port %u\n", rx_lcore_id, portid);
        }

        nb_mbufs = RTE_MAX(nb_ports
                * (nb_rxd + nb_txd + max_pkt_burst
                      + nb_lcores * MEMPOOL_CACHE_SIZE),
            8192U);

        /* create the mbuf pool
         * RTE_MBUF_DEFAULT_BUF_SIZE = (RTE_MBUF_DEFAULT_DATAROOM +
         * RTE_PKTMBUF_HEADROOM)
         * */
        l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", nb_mbufs,
            MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
        if (l2fwd_pktmbuf_pool == NULL)
                rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

        /*Initialise each port*/
        RTE_ETH_FOREACH_DEV(portid)
        {
                struct rte_eth_rxconf rxq_conf;
                struct rte_eth_txconf txq_conf;
                struct rte_eth_conf local_port_conf = port_conf;
                struct rte_eth_dev_info dev_info;

                /* skip ports that are not enabled */
                if ((l2fwd_enabled_port_mask & (1 << portid)) == 0) {
                        RTE_LOG(
                            INFO, USER1, "Skipping disabled port %u\n", portid);
                        continue;
                }

                /* init port */
                RTE_LOG(INFO, USER1, "Initializing port %u... \n", portid);
                fflush(stdout);
                rte_eth_dev_info_get(portid, &dev_info);
                if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
                        local_port_conf.txmode.offloads
                            |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;
                ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE,
                            "Cannot configure device: err=%d, port=%u\n", ret,
                            portid);

                ret = rte_eth_dev_adjust_nb_rx_tx_desc(
                    portid, &nb_rxd, &nb_txd);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE,
                            "Cannot adjust number of descriptors: err=%d, "
                            "port=%u\n",
                            ret, portid);

                /* [Zuo] Get MAC addresses of all enabled port */
                rte_eth_macaddr_get(portid, &l2fwd_ports_eth_addr[portid]);

                /* init one RX queue */
                fflush(stdout);
                rxq_conf = dev_info.default_rxconf;
                rxq_conf.offloads = local_port_conf.rxmode.offloads;
                ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
                    rte_eth_dev_socket_id(portid), &rxq_conf,
                    l2fwd_pktmbuf_pool);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE,
                            "rte_eth_rx_queue_setup:err=%d, port=%u\n", ret,
                            portid);

                /* init one TX queue on each port */
                fflush(stdout);
                txq_conf = dev_info.default_txconf;
                // txq_conf.txq_flags = ETH_TXQ_FLAGS_IGNORE;
                txq_conf.offloads = local_port_conf.txmode.offloads;
                ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
                    rte_eth_dev_socket_id(portid), &txq_conf);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE,
                            "rte_eth_tx_queue_setup:err=%d, port=%u\n", ret,
                            portid);

                /* Initialize TX buffers */
                tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
                    RTE_ETH_TX_BUFFER_SIZE(max_pkt_burst), 0,
                    rte_eth_dev_socket_id(portid));
                if (tx_buffer[portid] == NULL)
                        rte_exit(EXIT_FAILURE,
                            "Cannot allocate buffer for tx on port %u\n",
                            portid);

                rte_eth_tx_buffer_init(tx_buffer[portid], max_pkt_burst);

                /* Start device */
                ret = rte_eth_dev_start(portid);
                if (ret < 0)
                        rte_exit(EXIT_FAILURE,
                            "rte_eth_dev_start:err=%d, port=%u\n", ret, portid);
                RTE_LOG(INFO, USER1, "Device started for port: %d\n", portid);

                /* MARK: Enable promiscuous mode in each enabled port */
                /*rte_eth_promiscuous_enable(portid);*/

                RTE_LOG(INFO, USER1,
                    "Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                    portid, l2fwd_ports_eth_addr[portid].addr_bytes[0],
                    l2fwd_ports_eth_addr[portid].addr_bytes[1],
                    l2fwd_ports_eth_addr[portid].addr_bytes[2],
                    l2fwd_ports_eth_addr[portid].addr_bytes[3],
                    l2fwd_ports_eth_addr[portid].addr_bytes[4],
                    l2fwd_ports_eth_addr[portid].addr_bytes[5]);
        }

        check_all_ports_link_status(l2fwd_enabled_port_mask);

        ret = 0;
        /* launch per-lcore init on every lcore */
        rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, NULL, CALL_MASTER);

        RTE_LCORE_FOREACH_SLAVE(lcore_id)
        {
                if (rte_eal_wait_lcore(lcore_id) < 0) {
                        ret = -1;
                        break;
                }
        }

        /* Cleanups */
        RTE_LOG(INFO, EAL, "Run cleanups.\n");
        RTE_ETH_FOREACH_DEV(portid)
        {
                if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
                        continue;
                RTE_LOG(INFO, USER1, "Closing port %d...", portid);
                rte_eth_dev_stop(portid);
                rte_eth_dev_close(portid);
                RTE_LOG(INFO, USER1, " Done\n");
        }

        if (packet_capturing == 1) {
                rte_pdump_uninit();
        }

        RTE_LOG(INFO, EAL, "App exits.\n");
        RTE_LOG(INFO, EAL, "Release hugepages.\n");
        /* MARK: New feature supported by v18.02 */
        rte_eal_cleanup();

        return ret;
}
