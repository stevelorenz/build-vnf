/*
 * ipc.h
 */

#ifndef IPC_H
#define IPC_H

#include "collections.h"

/**
 * @file
 *
 * Inter-process Communication helpers
 * The goal is to use not only fast but also simple approach to handle the IPC
 * between DPDK fast path and slow path processing apps (e.g. neutral network
 * processing implemented with Tensorflow's Python API)
 *
 */

/************************
 *  Unix Domain Socket  *
 ************************/

/**
 * @brief init_uds_stream_cli
 *
 * @param socket_path
 *
 * @return
 */
int init_uds_stream_cli(const char *socket_path);

/**
 * @brief send_mbufs_data_uds
 *
 * @param socket
 * @param buf
 * @param size_b
 * @param nb_mbuf
 * @param tail_size
 */
void send_mbufs_data_uds(int socket, struct rte_mbuf **buf, uint32_t size_b,
			 uint16_t nb_mbuf, uint16_t tail_size);

/**
 * send_mvec_data_uds() - Send data room of all mbufs in a mbuf vector with Unix
 * domain socket.
 *
 * @param socket
 * @param v
 */
void send_mvec_data_uds(int socket, struct mvec *v);

/**
 * recv_mvec_data_uds() - Fill data room of all mbufs in a mbuf vector with data
 * received from an Unix domain socket.
 *
 * @param socket
 * @param v
 *
 * @return
 */
uint16_t recv_mvec_data_uds(int socket, struct mvec *v);

#endif /* !IPC_H */
