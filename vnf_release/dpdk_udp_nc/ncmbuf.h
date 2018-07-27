/*
 * ncmbuf.h
 *
 * Description: Network code DPDK mbuf with NCKernel
 */

#ifndef NCMBUF_H
#define NCMBUF_H

#endif /* !NCMBUF_H */

#include <stdint.h>
#include <stdio.h>

#include <dpdk_helper.h>
#include <nckernel.h>
#include <skb.h>

#define UDP_NC_PAYLOAD_HEADER_LEN 90

/**
 * @brief Check if the mbuf's data length is enough for encoding
 *
 * @param mbuf_pool
 * @param enc
 * @param header_size
 */
void check_mbuf_size(struct rte_mempool* mbuf_pool, struct nck_encoder* enc,
    uint16_t header_size);

/***********************
 *  Mbuf NC operaions  *
 ***********************/

uint8_t encode_udp(
    struct rte_mbuf* m_in, struct rte_mempool* mbuf_pool, uint16_t port_id);
