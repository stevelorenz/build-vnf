/*
 * test.c
 * About: Test ncmbuf
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

#include "ncmbuf.h"

#define NB_MBUF 2048
#define MBUF_DATA_SIZE 2048
#define MEMPOOL_CACHE_SIZE 256
#define MBUF_l2_HDR_LEN 14
#define MBUF_l3_HDR_LEN 20
#define MBUF_l4_UDP_HDR_LEN 8
#define TEST_PKT_NUM 128
#define TEST_BUFFER_SIZE 256
#define TEST_DATA_SIZE 1400
#define TEST_HDR_LEN 42
#define MAGIC_DATA 0x17

void put_test_rxq(struct rte_mbuf* m, uint16_t portid);
struct rte_mbuf* gen_test_udp(
    struct rte_mempool* mbuf_pool, uint16_t hdr_len, uint16_t data_len);

struct nck_option_value TEST_OPTION[]
    = { { "protocol", "noack" }, { "symbol_size", "1402" }, { "symbols", "2" },
              { "redundancy", "1" }, { NULL, NULL } };

void put_test_rxq(struct rte_mbuf* m, uint16_t portid) { rte_pktmbuf_free(m); }

struct rte_mbuf* gen_test_udp(
    struct rte_mempool* mbuf_pool, uint16_t hdr_len, uint16_t data_len)
{
        struct rte_mbuf* m;
        uint8_t* pt_data;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;

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
        udph->dgram_len = rte_cpu_to_be_16(data_len + MBUF_l4_UDP_HDR_LEN);
        iph->total_length = rte_cpu_to_be_16(
            data_len + MBUF_l4_UDP_HDR_LEN + MBUF_l3_HDR_LEN);
        iph->version_ihl = 0x45;
        return m;
}

int main(int argc, char** argv)
{
        int ret;
        struct rte_mempool* test_pktmbuf_pool;
        size_t i;
        struct rte_mbuf* source_buffer[TEST_PKT_NUM];
        uint64_t prev_tsc = 0;
        uint64_t cur_tsc = 0;

        ret = rte_eal_init(argc, argv);
        if (ret < 0) {
                rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
        }
        argc -= ret;
        argv += ret;

        test_pktmbuf_pool = rte_pktmbuf_pool_create("test_mbuf_pool", NB_MBUF,
            MEMPOOL_CACHE_SIZE, 0, MBUF_DATA_SIZE, rte_socket_id());

        if (test_pktmbuf_pool == NULL) {
                rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
        }

        struct nck_encoder enc;

        if (nck_create_encoder(
                &enc, NULL, TEST_OPTION, nck_option_from_array)) {
                rte_exit(EXIT_FAILURE, "Failed to create encoder.\n");
        }
        check_mbuf_size(test_pktmbuf_pool, &enc);

        for (i = 0; i < TEST_PKT_NUM; ++i) {
                source_buffer[i] = gen_test_udp(
                    test_pktmbuf_pool, TEST_HDR_LEN, TEST_DATA_SIZE);
        }

        /* Encoding operations */
        prev_tsc = rte_get_tsc_cycles();
        for (i = 0; i < TEST_PKT_NUM; ++i) {
                encode_udp(&enc, source_buffer[i], test_pktmbuf_pool, -1,
                    put_test_rxq);
        }
        cur_tsc = rte_get_tsc_cycles();
        printf("Number of TSCs for encoding: %lu\n", (cur_tsc - prev_tsc));

        RTE_LOG(INFO, EAL, "Run cleanups.\n");
        nck_free(&enc);
        rte_eal_cleanup();

        RTE_LOG(INFO, EAL, "Test exits.\n");
        return ret;
}
