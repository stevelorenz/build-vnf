/*
 * ipc.h
 */

#ifndef IPC_H
#define IPC_H

/**
 * @file
 *
 * Inter-process Communication helpers
 *
 */

/**
 * @brief init_uds_stream_cli
 *
 * @param socket_path
 *
 * @return
 */
int init_uds_stream_cli(const char* socket_path);

/**
 * @brief send_mbufs_data_uds
 *
 * @param socket
 * @param buf
 * @param size_b
 * @param nb_mbuf
 * @param tail_size
 */
void send_mbufs_data_uds(int socket, struct rte_mbuf** buf, uint32_t size_b,
    uint16_t nb_mbuf, uint16_t tail_size);

#endif /* !IPC_H */
