/*
 * test.c
 *
 * About: Test ncmbuf
 *
 *        - Some helper functions can be summarized for other tests
 */

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_per_lcore.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_udp.h>

#include <dpdk_helper.h>
#include <ncmbuf.h>

#define NB_MBUF 2048
#define MBUF_DATA_SIZE 2048
#define MEMPOOL_CACHE_SIZE 256
#define MBUF_l2_HDR_LEN 14
#define MBUF_l3_HDR_LEN 20
#define MBUF_l4_UDP_HDR_LEN 8

#define TEST_DATA_SIZE 128
#define TEST_HDR_LEN 42

#define MAGIC_DATA 0x17

#define UNCODED_BUFFER_LEN 2
#define CODED_BUFFER_LEN 3
#define DECODED_BUFFER_LEN 2

/* TODO:  <30-07-18,Zuo> Extend this to support multiple options */
struct nck_option_value TEST_OPTION[]
    = { { "protocol", "noack" }, { "symbol_size", "1402" }, { "symbols", "2" },
              { "redundancy", "1" }, { NULL, NULL } };

struct rte_mbuf* gen_test_udp(
    struct rte_mempool* mbuf_pool, uint16_t hdr_len, uint16_t data_len);

/* Not elegant... But currently no time for this... */
void put_coded_buffer(struct rte_mbuf* m, uint16_t portid);
void put_recoded_buffer(struct rte_mbuf* m, uint16_t portid);
void put_decoded_buffer(struct rte_mbuf* m, uint16_t portid);
void print_mbuf_buf(struct rte_mbuf* m_buf[], size_t buff_size);
/* TODO:  <30-07-18, Zuo> Use lists in sys/queue.h instead of fixed arrays
 * Move these test functions to dpdp_test in shared_lib.
 * */
struct rte_mbuf* UNCODED_BUFFER[UNCODED_BUFFER_LEN];
struct rte_mbuf* CODED_BUFFER[CODED_BUFFER_LEN];
struct rte_mbuf* RECODED_BUFFER[CODED_BUFFER_LEN];
struct rte_mbuf* DECODED_BUFFER[DECODED_BUFFER_LEN];

void put_coded_buffer(struct rte_mbuf* m, uint16_t portid)
{
        static uint16_t idx = 0;
        CODED_BUFFER[idx] = m;
        idx++;
}

void put_recoded_buffer(struct rte_mbuf* m, uint16_t portid)
{
        static uint16_t idx = 0;
        RECODED_BUFFER[idx] = m;
        idx++;
}

void put_decoded_buffer(struct rte_mbuf* m, uint16_t portid)
{
        static uint16_t idx = 0;
        DECODED_BUFFER[idx] = m;
        idx++;
}

void print_mbuf_buf(struct rte_mbuf* m_buf[], size_t buff_size)
{
        size_t i;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;

        for (i = 0; i < buff_size; ++i) {
                iph = rte_pktmbuf_mtod_offset(
                    m_buf[i], struct ipv4_hdr*, ETHER_HDR_LEN);
                udph = (struct udp_hdr*)((char*)iph + 20);
                printf("Index:%lu, udp dgram length:%u, ip total length: %u\n",
                    i, rte_be_to_cpu_16(udph->dgram_len),
                    rte_be_to_cpu_16(iph->total_length));
        }
}

struct rte_mbuf* gen_test_udp(
    struct rte_mempool* mbuf_pool, uint16_t hdr_len, uint16_t data_len)
{
        struct rte_mbuf* m;
        uint8_t* pt_data;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint64_t cur_tsc;

        m = rte_pktmbuf_alloc(mbuf_pool);
        if (m == NULL) {
                rte_exit(EXIT_FAILURE, "Can not allocate new mbuf for UDP\n");
        }
        m->data_len = hdr_len + data_len;
        m->pkt_len = hdr_len + data_len;
        pt_data = rte_pktmbuf_mtod(m, uint8_t*);
        printf("[GEN] Header room: %u, pt_data offset: %ld, data offset:%d\n",
            rte_pktmbuf_headroom(m), pt_data - (uint8_t*)(m->buf_addr),
            m->data_off);
        memset(pt_data, MAGIC_DATA, hdr_len + data_len);

        iph = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph + 20);
        cur_tsc = rte_get_tsc_cycles();
        pt_data = (uint8_t*)udph + 8;
        rte_memcpy(pt_data, &cur_tsc, sizeof(uint64_t));
        udph->dgram_len = rte_cpu_to_be_16(data_len + MBUF_l4_UDP_HDR_LEN);
        iph->total_length = rte_cpu_to_be_16(
            data_len + MBUF_l4_UDP_HDR_LEN + MBUF_l3_HDR_LEN);
        iph->version_ihl = 0x45;
        px_mbuf_udp(m);
        return m;
}

int main(int argc, char** argv)
{
        int ret;
        struct rte_mempool* test_pktmbuf_pool;
        size_t i;
        uint64_t prev_tsc = 0;
        uint64_t cur_tsc = 0;

        ret = rte_eal_init(argc, argv);
        if (ret < 0) {
                rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
        }
        argc -= ret;
        argv += ret;

        rte_log_set_global_level(RTE_LOG_DEBUG);
        rte_log_set_level(RTE_LOGTYPE_USER1, RTE_LOG_DEBUG);

        test_pktmbuf_pool = rte_pktmbuf_pool_create("test_mbuf_pool", NB_MBUF,
            MEMPOOL_CACHE_SIZE, 0, MBUF_DATA_SIZE, rte_socket_id());

        if (test_pktmbuf_pool == NULL) {
                rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
        }

        struct nck_encoder enc;
        struct nck_recoder rec;
        struct nck_decoder dec;

        if (nck_create_encoder(
                &enc, NULL, TEST_OPTION, nck_option_from_array)) {
                rte_exit(EXIT_FAILURE, "Failed to create encoder.\n");
        }
        check_mbuf_size(test_pktmbuf_pool, &enc, TEST_HDR_LEN);

        if (nck_create_recoder(
                &rec, NULL, TEST_OPTION, nck_option_from_array)) {
                rte_exit(EXIT_FAILURE, "Failed to create recoder.\n");
        }

        if (nck_create_decoder(
                &dec, NULL, TEST_OPTION, nck_option_from_array)) {
                rte_exit(EXIT_FAILURE, "Failed to create decoder.\n");
        }

        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                UNCODED_BUFFER[i] = gen_test_udp(
                    test_pktmbuf_pool, TEST_HDR_LEN, TEST_DATA_SIZE);
        }
        printf("------- Before Encoding ------\n");

        /* Encoding operations */
        prev_tsc = rte_get_tsc_cycles();
        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                encode_udp_data(&enc, UNCODED_BUFFER[i], test_pktmbuf_pool, -1,
                    put_coded_buffer);
        }
        cur_tsc = rte_get_tsc_cycles();
        printf("------- After Encoding ------\n");
        printf(
            "[TIME] Number of TSCs for encoding: %lu\n", (cur_tsc - prev_tsc));

        /* Recoding operations */
        // prev_tsc = rte_get_tsc_cycles();
        // for (i = 0; i < CODED_BUFFER_LEN; ++i) {
        //         recode_udp_data(&rec, CODED_BUFFER[i], test_pktmbuf_pool, -1,
        //             put_recoded_buffer);
        // }
        // cur_tsc = rte_get_tsc_cycles();
        // printf(
        //     "[TIME] Number of TSCs for recoding: %lu\n", (cur_tsc -
        //     prev_tsc));

        // Simulate losses

        /* Decoding operations */
        printf("------- Before Decoding ------\n");
        prev_tsc = rte_get_tsc_cycles();
        for (i = 0; i < CODED_BUFFER_LEN; ++i) {
                decode_udp_data(&dec, CODED_BUFFER[i], test_pktmbuf_pool, -1,
                    put_decoded_buffer);
        }
        cur_tsc = rte_get_tsc_cycles();
        printf("------- After Decoding ------\n");
        printf(
            "[TIME] Number of TSCs for decoding: %lu\n", (cur_tsc - prev_tsc));

        /* Compare UNCODED_BUFFER and CODED_BUFFER */
        uint8_t* pt_uncoded;
        uint8_t* pt_decoded;
        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                px_mbuf_udp(DECODED_BUFFER[i]);
                pt_uncoded = rte_pktmbuf_mtod(UNCODED_BUFFER[i], uint8_t*);
                pt_decoded = rte_pktmbuf_mtod(DECODED_BUFFER[i], uint8_t*);
                if (memcmp(
                        pt_uncoded, pt_decoded, TEST_HDR_LEN + TEST_DATA_SIZE)
                    != 0) {
                        printf("[TEST] Index:%lu, decoding failed\n", i);
                        return -1;
                }
        }
        printf("[TEST] Decoding succeeded!\n");

        RTE_LOG(INFO, EAL, "Run cleanups.\n");
        nck_free(&enc);
        nck_free(&rec);
        nck_free(&dec);
        rte_eal_cleanup();

        RTE_LOG(INFO, EAL, "Test exits.\n");
        return ret;
}
