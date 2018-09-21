/*
 * eval.c
 *
 * About: Latency evaluations for advanced networking functions
 *
 *        - AES encryption and decryption with CTR mode
 *
 *        - Network Encoding
 *
 *              - Protocol: NOACK and Sliding window
 *              - Field size: binary, binary8
 *              - Symbols: 32 redundancy: 8 -> 25%
 *
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

#define TEST_DATA_SIZE 1400
#define TEST_HDR_LEN 42

#define MAGIC_DATA 0x17

#define UNCODED_BUFFER_LEN 320
#define CODED_BUFFER_LEN 400
#define DECORYPTED_BUFFER_LEN 320

#ifndef EVAL_PROFILE
#define EVAL_PROFILE 0
#endif

uint16_t SYMBOLS = 32;
uint16_t REDUNDANCY = 8;

struct rte_mbuf* gen_test_udp(
    struct rte_mempool* mbuf_pool, uint16_t hdr_len, uint16_t data_len);

/* Not elegant... But currently no time for this... */
void put_coded_buffer(struct rte_mbuf* m, uint16_t portid);
void put_decrypted_buffer(struct rte_mbuf* m, uint16_t portid);
void fill_select_pkts(uint16_t array[], uint16_t len, uint16_t st);
void print_sel_pkts(uint16_t array[], uint16_t len);

struct rte_mbuf* ORIGINAL_BUFFER[UNCODED_BUFFER_LEN];
struct rte_mbuf* UNCODED_BUFFER[UNCODED_BUFFER_LEN];
struct rte_mbuf* CODED_BUFFER[CODED_BUFFER_LEN];
struct rte_mbuf* DECRYPTED_BUFFER[DECORYPTED_BUFFER_LEN];

void put_coded_buffer(struct rte_mbuf* m, uint16_t portid)
{
        static uint16_t idx = 0;
        CODED_BUFFER[idx] = m;
        idx++;
}

void put_decrypted_buffer(struct rte_mbuf* m, uint16_t portid)
{
        static uint16_t idx = 0;
        DECRYPTED_BUFFER[idx] = m;
        idx++;
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
        recalc_cksum(iph, udph);
        return m;
}

int main(int argc, char** argv)
{
        int ret;
        struct rte_mempool* test_pktmbuf_pool;
        size_t i;
        uint64_t prev_tsc = 0;
        uint64_t cur_tsc = 0;
        double delay_arr[UNCODED_BUFFER_LEN] = { 0.0 };
        FILE* f = NULL;
        uint8_t succ = 0;

        ret = rte_eal_init(argc, argv);
        if (ret < 0) {
                rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
        }
        argc -= ret;
        argv += ret;

        rte_log_set_global_level(RTE_LOG_DEBUG);
        rte_log_set_level(RTE_LOGTYPE_USER1, RTE_LOG_INFO);

        test_pktmbuf_pool = rte_pktmbuf_pool_create("test_mbuf_pool", NB_MBUF,
            MEMPOOL_CACHE_SIZE, 0, MBUF_DATA_SIZE, rte_socket_id());

        if (test_pktmbuf_pool == NULL) {
                rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
        }

#if defined(EVAL_PROFILE) && (EVAL_PROFILE == 0)
        RTE_LOG(INFO, EAL, "Evaluate NC encoding: NOACK. \n");

        struct nck_option_value test_option[] = {
                { "field", "binary8" },
                { "protocol", "noack" },
                { "symbol_size", "1402" },
                { "symbols", "32" },
                { "redundancy", "8" },
                { NULL, NULL },
        };
#endif

#if defined(EVAL_PROFILE) && (EVAL_PROFILE == 1)
        RTE_LOG(INFO, EAL, "Evaluate NC encoding: Sliding Window. \n");

        struct nck_option_value test_option[] = { { "field", "binary8" },
                { "protocol", "sliding_window" }, { "symbol_size", "1402" },
                { "symbols", "32" }, { "redundancy", "8" }, { NULL, NULL },
                { "feedback", "0" } };
#endif

#if defined(EVAL_PROFILE) && ((EVAL_PROFILE == 0) || (EVAL_PROFILE == 1))
        struct nck_encoder enc;

        if (nck_create_encoder(
                &enc, NULL, test_option, nck_option_from_array)) {
                rte_exit(EXIT_FAILURE, "Failed to create encoder.\n");
        }
        check_mbuf_size(test_pktmbuf_pool, &enc, TEST_HDR_LEN);

        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                UNCODED_BUFFER[i] = gen_test_udp(
                    test_pktmbuf_pool, TEST_HDR_LEN, TEST_DATA_SIZE);
                ORIGINAL_BUFFER[i] = mbuf_udp_deep_copy(UNCODED_BUFFER[i],
                    test_pktmbuf_pool, (TEST_HDR_LEN + TEST_DATA_SIZE));
        }

        prev_tsc = rte_get_tsc_cycles();
        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                encode_udp_data_delay(&enc, UNCODED_BUFFER[i],
                    test_pktmbuf_pool, -1, put_coded_buffer, &(delay_arr[i]));
        }
        cur_tsc = rte_get_tsc_cycles();

        printf("[TIME] Total time used for encoding: %f ms\n",
            (1.0 / rte_get_timer_hz()) * 1000.0 * (cur_tsc - prev_tsc));

        f = fopen("per_packet_delay_nc.csv", "a+");

        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                fprintf(f, "%f\n", delay_arr[i]);
        }
        fclose(f);
#endif

        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                if (UNCODED_BUFFER[i] != NULL) {
                        rte_pktmbuf_free(UNCODED_BUFFER[i]);
                }
                if (ORIGINAL_BUFFER[i] != NULL) {
                        rte_pktmbuf_free(ORIGINAL_BUFFER[i]);
                }
        }

        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                UNCODED_BUFFER[i] = gen_test_udp(
                    test_pktmbuf_pool, TEST_HDR_LEN, TEST_DATA_SIZE);
                ORIGINAL_BUFFER[i] = mbuf_udp_deep_copy(UNCODED_BUFFER[i],
                    test_pktmbuf_pool, (TEST_HDR_LEN + TEST_DATA_SIZE));
        }

        RTE_LOG(INFO, EAL, "Evaluate AES CTR encryption.\n");

        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                aes_ctr_xcrypt_udp_data_delay(UNCODED_BUFFER[i],
                    test_pktmbuf_pool, -1, NULL, &(delay_arr[i]));
        }

        f = fopen("per_packet_delay_aes_ctr_enc.csv", "a+");
        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                fprintf(f, "%f\n", delay_arr[i]);
        }
        fclose(f);

        RTE_LOG(INFO, EAL, "Evaluate AES CTR decryption.\n");
        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                aes_ctr_xcrypt_udp_data_delay(UNCODED_BUFFER[i],
                    test_pktmbuf_pool, -1, put_decrypted_buffer,
                    &(delay_arr[i]));
        }

        f = fopen("per_packet_delay_aes_ctr_dec.csv", "a+");
        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                fprintf(f, "%f\n", delay_arr[i]);
        }
        fclose(f);

        succ = 1;
        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                ret = mbuf_data_cmp(ORIGINAL_BUFFER[i], DECRYPTED_BUFFER[i]);
                if (ret != 0) {
                        succ = 0;
                        printf("[ERR] Packet %lu failed to be decoded.\n", i);
                        printf("------ Original Packet: \n");
                        px_mbuf_udp(ORIGINAL_BUFFER[i]);
                        printf("------ Decrypted Packet: \n");
                        px_mbuf_udp(DECRYPTED_BUFFER[i]);
                        break;
                }
        }
        if (succ == 1) {
                printf("^_^ Decryption successfully!\n");
        }

        RTE_LOG(INFO, EAL, "Run cleanups.\n");
        nck_free(&enc);
        rte_eal_cleanup();

        RTE_LOG(INFO, EAL, "Evaluation exits.\n");
        return ret;
}
