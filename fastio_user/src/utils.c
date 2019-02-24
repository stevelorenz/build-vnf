/*
 * dpdk_test.c
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

#include "utils.h"

struct rte_mbuf* mbuf_udp_deep_copy(
    struct rte_mbuf* m, struct rte_mempool* mbuf_pool, uint16_t hdr_len)
{
        if (m->nb_segs > 1) {
                RTE_LOG(ERR, USER1,
                    "Deep copy doest not support scattered segments.\n");
                return NULL;
        }
        struct rte_mbuf* m_copy;
        m_copy = rte_pktmbuf_alloc(mbuf_pool);
        m_copy->data_len = m->data_len;
        m_copy->pkt_len = m->pkt_len;
        rte_memcpy(rte_pktmbuf_mtod(m_copy, uint8_t*),
            rte_pktmbuf_mtod(m, uint8_t*), hdr_len);
        return m_copy;
}

void recalc_cksum(struct ipv4_hdr* iph, struct udp_hdr* udph)
{
        udph->dgram_cksum = 0;
        iph->hdr_checksum = 0;
        udph->dgram_cksum = rte_ipv4_udptcp_cksum(iph, udph);
        iph->hdr_checksum = rte_ipv4_cksum(iph);
}

void px_mbuf_udp(struct rte_mbuf* m)
{
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint8_t* pt_data;
        uint16_t data_len;
        size_t i;
        printf("Dataroom len: %u\n", m->data_len);

        printf("Header part: \n");
        pt_data = rte_pktmbuf_mtod(m, uint8_t*);
        for (i = 0; i < (14 + 20 + 8); ++i) {
                printf("%02x ", *(pt_data + i));
        }
        printf("\n");
        iph = rte_pktmbuf_mtod_offset(m, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph + 20);
        data_len = rte_be_to_cpu_16(udph->dgram_len) - 8;
        // printf("[PRINT] UDP dgram len:%u, data len:%u\n",
        // rte_be_to_cpu_16(udph->dgram_len), data_len);
        pt_data = (uint8_t*)udph + 8;
        printf("UDP data: \n");
        for (i = 0; i < data_len; ++i) {
                printf("%02x ", *(pt_data + i));
        }
        printf("\n");
}

int mbuf_data_cmp(struct rte_mbuf* m1, struct rte_mbuf* m2)
{
        uint8_t* pt_m1;
        uint8_t* pt_m2;
        size_t i;

        pt_m1 = rte_pktmbuf_mtod(m1, uint8_t*);
        pt_m2 = rte_pktmbuf_mtod(m2, uint8_t*);
        if (m1->data_len != m2->data_len) {
                RTE_LOG(INFO, USER1,
                    "Mbufs have different data_len. m1 len: %u, m2 len: %u\n",
                    m1->data_len, m2->data_len);
                return -2;
        }

        for (i = 0; i < m1->data_len; ++i) {
                if (*pt_m1 < *pt_m2) {
                        return -1;
                } else if (*pt_m1 > *pt_m2) {
                        return 1;
                }
        }
        return 0;
}

int mbuf_udp_cmp(struct rte_mbuf* m1, struct rte_mbuf* m2)
{
        /*uint8_t* pt_m1;*/
        /*uint8_t* pt_m2;*/

        rte_pktmbuf_free(m1);
        rte_pktmbuf_free(m2);

        return 0;
}
