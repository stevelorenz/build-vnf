/*
 * dpdk_test.h
 *
 * About: Helper functions for DPDK development
 *
 *        These are util functions that are not provided by official DPDK APIs
 *        (Version 18.02).
 *
 * Email: xianglinks@gmail.com
 */

#ifndef DPDK_HELPER_H
#define DPDK_HELPER_H

/**
 * @brief Make a deep copy of a mbuf
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
 * @brief Recalculate IP and UDP checksum
 *
 * @param iph
 * @param udph
 */
void recalc_cksum(struct ipv4_hdr* iph, struct udp_hdr* udph);

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
int mbuf_udp_cmp(struct rte_mbuf* m1, struct rte_mbuf* m2);

/**
 * @brief mbuf_dump_bin
 *
 * @param f
 * @param m
 */
void mbuf_dump_bin(struct rte_mbuf* m);

#endif /* !DPDK_HELPER_H */

