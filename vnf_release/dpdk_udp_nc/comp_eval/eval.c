/*
 * eval.c
 *
 * About: Latency evaluations for advanced networking functions
 *
 *        - AES encryption and decryption with CTR mode
 *
 *              - AES128 (key[16]) + AES192(key[24]) + AES256 (key[32])
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
#define SYMBOL_SIZE "1402"

#define MAGIC_DATA 0x17

/* MARK: increasing this value requires more hugepage memory */
#define UNCODED_BUFFER_LEN 320

#define NC_PROFILE_NUM 4

#define CTR 1
#define CBC 0
#define ECB 0
#define AES256 1
#define AES192 0
#define AES128 0

struct rte_mbuf* gen_test_udp(
    struct rte_mempool* mbuf_pool, uint16_t hdr_len, uint16_t data_len);

void put_decrypted_buffer(struct rte_mbuf* m, uint16_t portid);

struct rte_mbuf* ORIGINAL_BUFFER[UNCODED_BUFFER_LEN] = { NULL };
struct rte_mbuf* UNCODED_BUFFER[UNCODED_BUFFER_LEN] = { NULL };
struct rte_mbuf* DECRYPTED_BUFFER[UNCODED_BUFFER_LEN] = { NULL };

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
        size_t j;
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

        RTE_LOG(INFO, EAL, "[EVAL] Evaluate NC encoding: \n");

        struct nck_option_value test_options[NC_PROFILE_NUM][7] = {

                { { "field", "binary" }, { "protocol", "noack" },
                    { "symbol_size", SYMBOL_SIZE }, { "symbols", "32" },
                    { "redundancy", "8" }, { NULL, NULL }, { NULL, NULL } },

                { { "field", "binary" }, { "protocol", "sliding_window" },
                    { "symbol_size", SYMBOL_SIZE }, { "symbols", "32" },
                    { "redundancy", "8" }, { NULL, NULL },
                    { "feedback", "0" } },

                { { "field", "binary8" }, { "protocol", "noack" },
                    { "symbol_size", SYMBOL_SIZE }, { "symbols", "32" },
                    { "redundancy", "8" }, { NULL, NULL }, { NULL, NULL } },
                { { "field", "binary8" }, { "protocol", "sliding_window" },
                    { "symbol_size", SYMBOL_SIZE }, { "symbols", "32" },
                    { "redundancy", "8" }, { NULL, NULL }, { "feedback", "0" } }

        };

        for (j = 0; j < NC_PROFILE_NUM; ++j) {
                struct nck_encoder enc;

                if (nck_create_encoder(
                        &enc, NULL, test_options[j], nck_option_from_array)) {
                        rte_exit(EXIT_FAILURE, "Failed to create encoder.\n");
                }
                check_mbuf_size(test_pktmbuf_pool, &enc, TEST_HDR_LEN);

                for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                        UNCODED_BUFFER[i] = gen_test_udp(
                            test_pktmbuf_pool, TEST_HDR_LEN, TEST_DATA_SIZE);
                        ORIGINAL_BUFFER[i] = mbuf_udp_deep_copy(
                            UNCODED_BUFFER[i], test_pktmbuf_pool,
                            (TEST_HDR_LEN + TEST_DATA_SIZE));
                }

                prev_tsc = rte_get_tsc_cycles();
                for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                        encode_udp_data_delay(&enc, UNCODED_BUFFER[i],
                            test_pktmbuf_pool, -1, NULL, &(delay_arr[i]));
                }
                cur_tsc = rte_get_tsc_cycles();

                printf("[TIME] Total time used for encoding: %f ms\n",
                    (1.0 / rte_get_timer_hz()) * 1000.0 * (cur_tsc - prev_tsc));

                if (j == 0) {
                        f = fopen("per_packet_delay_nc_enc_noack_20_binary.csv",
                            "a+");
                } else if (j == 1) {
                        f = fopen("per_packet_delay_nc_enc_sliding_window_20_"
                                  "binary.csv",
                            "a+");
                } else if (j == 2) {
                        f = fopen(
                            "per_packet_delay_nc_enc_noack_20_binary8.csv",
                            "a+");
                } else if (j == 3) {
                        f = fopen("per_packet_delay_nc_enc_sliding_window_20_"
                                  "binary8.csv",
                            "a+");
                } else {
                        rte_exit(
                            EXIT_FAILURE, "Unknown NC evaluation options.\n");
                }

                for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                        fprintf(f, "%f\n", delay_arr[i]);
                }

                fclose(f);
                nck_free(&enc);

                for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                        if (UNCODED_BUFFER[i] != NULL) {
                                rte_pktmbuf_free(UNCODED_BUFFER[i]);
                        }
                        if (ORIGINAL_BUFFER[i] != NULL) {
                                rte_pktmbuf_free(ORIGINAL_BUFFER[i]);
                        }
                }
        }

        RTE_LOG(INFO, EAL, "Evaluate AES CTR encryption and decryption.\n");

#if defined(AES256)
        printf("\nTesting AES256\n\n");
#elif defined(AES192)
        printf("\nTesting AES192\n\n");
#elif defined(AES128)
        printf("\nTesting AES128\n\n");
#else
        printf("You need to specify a symbol between AES128, AES192 or AES256. "
               "Exiting");
        return 0;
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

        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                aes_ctr_xcrypt_udp_data_delay(
                    UNCODED_BUFFER[i], -1, NULL, &(delay_arr[i]));
        }

        f = fopen("per_packet_delay_aes_ctr_enc_20_256.csv", "a+");
        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                fprintf(f, "%f\n", delay_arr[i]);
        }
        fclose(f);

        for (i = 0; i < UNCODED_BUFFER_LEN; ++i) {
                aes_ctr_xcrypt_udp_data_delay(UNCODED_BUFFER[i], -1,
                    put_decrypted_buffer, &(delay_arr[i]));
        }

        f = fopen("per_packet_delay_aes_ctr_dec_20_256.csv", "a+");
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
        rte_eal_cleanup();

        RTE_LOG(INFO, EAL, "Evaluation exits.\n");
        return ret;
}
