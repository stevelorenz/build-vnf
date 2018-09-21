/*
 * ncmbuf.h
 *
 * Description: Network code DPDK mbuf with NCKernel
 *
 *    Features:
 *              - Efficient coding of UDP data
 *
 * Limitations:
 *              - TODO: Jumbo frames are not supported
 *
 *       Email: xianglinks@gmail.com
 */

#ifndef NCMBUF_H
#define NCMBUF_H

#include <stdint.h>

#include <nckernel.h>
#include <skb.h>

/* TODO:  <01-08-18,Zuo> */
// Add get_nc_func(uint16_t coder_type)

/**
 * @brief Check if the mbuf's data length is enough for encoding
 *      The data room of input mbuf is used as coding buffer to exchange data
 *      with coder's buffer. The data room MUST be larger than coder.coded_size
 *      plus reserved header length.
 *
 * @param mbuf_pool
 * @param enc
 * @param hdr_len
 */
void check_mbuf_size(
    struct rte_mempool* mbuf_pool, struct nck_encoder* enc, uint16_t hdr_len);

/**
 * @brief Make a deep copy of a in mbuf encapsulated UDP segment.
 *        Only UDP header is copied, the payload should be appended latter.
 *
 * @param m
 * @param mbuf_pool
 * @param hdr_len
 *
 * @return
 */
struct rte_mbuf* mbuf_udp_deep_copy(
    struct rte_mbuf* m, struct rte_mempool* mbuf_pool, uint16_t hdr_len);

/***********************
 *  Mbuf NC operaions  *
 ***********************/

/**
 * @brief Encode UDP segments
 *
 * @param enc
 * @param m_in
 * @param mbuf_pool
 * @param portid
 * @param put_rxq
 *
 * @return
 */
uint8_t encode_udp_data(struct nck_encoder* enc, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t));

/**
 * @brief Encode UDP segments with monitoring per-packet latency
 *
 * @return
 */
uint8_t encode_udp_data_delay(struct nck_encoder* enc, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t), double* delay_val);

/**
 * @brief recode_udp_data
 *
 * @param rec
 * @param m_in
 * @param mbuf_pool
 * @param portid
 * @param put_rxq
 *
 * @return
 */
uint8_t recode_udp_data(struct nck_recoder* rec, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t));

/**
 * @brief decode_udp_data
 *
 * @param dec
 * @param m_in
 * @param mbuf_pool
 * @param portid
 * @param put_rxq
 *
 * @return
 */
uint8_t decode_udp_data(struct nck_decoder* dec, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t));

/**
 * @brief aes_encrpt_udp_data
 *
 * @param enc
 * @param m_in
 * @param mbuf_pool
 * @param portid
 * @param put_rxq
 *
 * @return
 */
uint8_t aes_ctr_xcrypt_udp_data(struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t));

uint8_t aes_ctr_xcrypt_udp_data_delay(struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t), double* delay_val);

#endif /* !NCMBUF_H */
