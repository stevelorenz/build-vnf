/*
 * utils.h
 */

#ifndef DPDK_HELPER_H
#define DPDK_HELPER_H

/**
 * @file
 *
 * Utility functions
 *
 */

/*********************
 *  Mbuf Operations  *
 *********************/

/**
 * mbuf_udp_deep_copy() -  Make a deep copy of a mbuf.
 *
 *
 * @param m
 * @param mbuf_pool
 * @param hdr_len
 *
 * @return
 */
struct rte_mbuf* mbuf_udp_deep_copy(
    struct rte_mbuf* m, struct rte_mempool* mbuf_pool, uint16_t hdr_len);

/**
 * mbuf_datacmp() - Compare two mbufs' data room
 *
 * @param m1
 * @param m2
 *
 * @return
 */
int mbuf_datacmp(struct rte_mbuf* m1, struct rte_mbuf* m2);

#endif /* !DPDK_HELPER_H */
