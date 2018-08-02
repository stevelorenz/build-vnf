/*
 * dpdk_test.h
 *
 * About: Helper functions to test DPDK App with simulations
 *
 */

#ifndef DPDK_TEST_H
#define DPDK_TEST_H

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

struct rte_mempool* init_mempool()
{

        struct rte_mempool* mp;

        mp = rte_pktmbuf_pool_create(
            "test_mempool", 2048, 256, 0, 2048, rte_socket_id());

        if (mp == NULL) {
                rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
        }
}

struct rte_mbuf* mbuf_udp_deep_copy(
    struct rte_mbuf* m, struct rte_mempool* mbuf_pool, uint16_t hdr_len)
{
        if (m->nb_segs > 1) {
                RTE_LOG(ERR, NCMBUF,
                    "Deep copy doest not support scattered segments.\n");
                return NULL;
        }
        struct rte_mbuf* m_copy;
        m_copy = rte_pktmbuf_alloc(mbuf_pool);
        m_copy->data_len = hdr_len;
        m_copy->pkt_len = hdr_len;
        rte_memcpy(rte_pktmbuf_mtod(m_copy, uint8_t*),
            rte_pktmbuf_mtod(m, uint8_t*), hdr_len);
        return m_copy;
}

#endif /* !DPDK_TEST_H */
