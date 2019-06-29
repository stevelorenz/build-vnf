/*
 * utils.c
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
#include <rte_prefetch.h>
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

int mbuf_datacmp(struct rte_mbuf* m1, struct rte_mbuf* m2)
{
        uint8_t* pt_m1 = NULL;
        uint8_t* pt_m2 = NULL;
        uint16_t len = 0;
        size_t i = 0;
        len = RTE_MIN(m1->data_len, m2->data_len);
        pt_m1 = rte_pktmbuf_mtod(m1, uint8_t*);
        pt_m2 = rte_pktmbuf_mtod(m2, uint8_t*);
        rte_prefetch_non_temporal((void*)pt_m1);
        rte_prefetch_non_temporal((void*)pt_m2);
        for (i = 0; i < len; ++i) {
                if (*pt_m1 < *pt_m2) {
                        return -1;
                } else if (*pt_m1 > *pt_m2) {
                        return 1;
                }
        }
        return 0;
}

double get_delay_tsc(uint64_t tsc_cnt)
{
        double delay = 0;
        delay = (1.0 / rte_get_tsc_hz()) * (tsc_cnt);
        return delay;
}
