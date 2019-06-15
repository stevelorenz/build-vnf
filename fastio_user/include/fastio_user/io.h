/*
 * io.h
 */

#ifndef IO_H
#define IO_H

/**
 * @file
 *
 * IO functions
 *
 */

#include <rte_mbuf.h>

/**
 * @brief dpdk_recv_into
 *
 * @param port_id
 * @param queue_id
 * @param rx_buf
 * @param offset
 * @param rx_nb
 */
/* MARK: Names of mbuf and rx_buf (an array of mbufs)) can be ambiguous, may
 * rename rx_buf for better APIs */
void dpdk_recv_into(uint16_t port_id, uint16_t queue_id,
    struct rte_mbuf** rx_buf, uint16_t offset, uint16_t rx_nb);

/**
 * @brief dpdk_free_buf
 *
 * @param buf
 * @param buf_size
 */
void dpdk_free_buf(struct rte_mbuf** buf, uint16_t buf_size);

/**************************
 *  Func for local Tests  *
 **************************/

uint16_t gen_rx_buf_from_file(char const* pathname, struct rte_mbuf** rx_buf,
    uint16_t rx_buf_size, struct rte_mempool* pool, uint16_t MTU,
    uint16_t* tail_size);

#endif /* !IO_H */
