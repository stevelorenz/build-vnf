/*
 * dpdk_test.h
 *
 * About: Helper functions to test DPDK App with simulations
 * Email: xianglinks@gmail.com
 */

#ifndef DPDK_HELPER_H
#define DPDK_HELPER_H

struct rte_mbuf* mbuf_udp_deep_copy(
    struct rte_mbuf* m, struct rte_mempool* mbuf_pool, uint16_t hdr_len);

struct rte_mempool* init_mempool(void);

/**
 * @brief Print in mbuf encapsulated UDP segment in hex format
 *
 */
void px_mbuf_udp(struct rte_mbuf* m);

/**
 * @brief Compare data room of two mbufs
 *
 * @param m1
 * @param m2
 *
 * @return
 */
int mbuf_data_cmp(struct rte_mbuf* m1, struct rte_mbuf* m2);

/**
 * @brief Compare two in mbuf encapsulated UDP segments
 *
 * @param m1
 * @param m2
 */
void mbuf_udp_cmp(struct rte_mbuf* m1, struct rte_mbuf* m2);

#endif /* !DPDK_HELPER_H */
