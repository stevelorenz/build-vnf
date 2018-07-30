/*
 * ncmbuf.h
 *
 * Description: Network code DPDK mbuf with NCKernel
 */

#ifndef NCMBUF_H
#define NCMBUF_H

#include <stdint.h>

#include <nckernel.h>
#include <skb.h>

#define UDP_NC_MAX_DATA_LEN 1500
#define UDP_NC_DATA_HEADER_LEN 90

/**
 * @brief Check if the mbuf's data length is enough for encoding
 *      The data room of input mbuf is used as coding buffer to exchange data
 *      with coder's buffer. The data room MUST be larger than coder.coded_size
 *      plus reserved header length.
 *
 * @param mbuf_pool
 * @param enc
 * @param header_size
 */
void check_mbuf_size(struct rte_mempool* mbuf_pool, struct nck_encoder* enc);

/***********************
 *  Mbuf NC operaions  *
 ***********************/

uint8_t encode_udp(struct nck_encoder* enc, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t));

uint8_t recode_udp(struct nck_recoder* rec, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t));

uint8_t decode_udp(struct nck_decoder* dec, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t));

#endif /* !NCMBUF_H */
