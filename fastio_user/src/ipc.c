/*
 * ipc.c
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <rte_byteorder.h>
#include <rte_config.h>
#include <rte_log.h>
#include <rte_mbuf.h>

#include "collections.h"
#include "ipc.h"

int init_uds_stream_cli(const char* socket_path)
{
        struct sockaddr_un addr;
        int sock;

        if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
                rte_exit(EXIT_FAILURE, "Can not create unix domain socket\n");
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
                rte_exit(EXIT_FAILURE,
                    "Can not connect to server unix domain socket\n");
        }

        return sock;
}

void send_mbufs_data_uds(int socket, struct rte_mbuf** buf, uint32_t size_b,
    uint16_t nb_mbuf, uint16_t tail_size)
{
        uint16_t i;
        uint32_t conv;
        uint8_t* data;

        // Send the total length with 4 bytes
        conv = rte_cpu_to_be_32(size_b);
        data = (uint8_t*)(&conv);
        send(socket, data, sizeof(uint32_t), 0);
        for (i = 0; i < nb_mbuf - 1; ++i) {
                data = rte_pktmbuf_mtod((*(buf + i)), uint8_t*);
                send(socket, data, (*(buf + i))->data_len, 0);
        }
        data = rte_pktmbuf_mtod((*(buf + nb_mbuf - 1)), uint8_t*);
        send(socket, data, tail_size, 0);
}

void send_mvec_data_uds(int socket, struct mvec* v)
{
        uint8_t i;
        uint8_t* data;
        uint32_t total_len = 0;

        // Send the total length with 4 bytes
        MVEC_FOREACH_MBUF(i, v) { total_len += (*(v->head + i))->data_len; }
        total_len = rte_cpu_to_be_32(total_len);
        data = (uint8_t*)(&total_len);
        send(socket, data, sizeof(uint32_t), 0);
        MVEC_FOREACH_MBUF(i, v)
        {
                data = rte_pktmbuf_mtod(*(v->head + i), uint8_t*);
                send(socket, data, (*(v->head + i))->data_len, 0);
        }
}

/**
 * sock_recvn() - Receive n bytes into buf with block recv function.
 *
 * @param socket
 * @param buf
 * @param len
 *
 */
static __rte_always_inline void sock_recvn(int socket, void* buf, uint16_t n)
{
        uint8_t* d;
        uint16_t r, tr = 0;

        d = (uint8_t*)(buf);
        while (tr < n) {
                r = recv(socket, d + tr, n - tr, 0);
                if (r <= 0) {
                        break;
                }
                tr += r;
        }
        if (tr != n) {
                /* Bad things happened, no clue...*/
                rte_exit(EXIT_FAILURE, "Can not read enough bytes from UDS.\n");
        }
}

uint16_t recv_mvec_data_uds(int socket, struct mvec* v)
{
        uint16_t i;
        uint16_t nb_mbuf = 0;
        uint16_t block_size = 0;
        uint32_t to_read = 0;

        sock_recvn(socket, &to_read, 4);
        to_read = rte_be_to_cpu_32(to_read);
        MVEC_FOREACH_MBUF(i, v)
        {
                block_size = (*(v->head + i))->data_len;
                /* The tail packet, the data room of this mbuf should be updated
                 */
                if (to_read < block_size) {
                        sock_recvn(socket,
                            rte_pktmbuf_mtod(*(v->head + i), void*), to_read);
                        rte_pktmbuf_trim(
                            *(v->head + i), (block_size - to_read));
                        to_read = 0;
                        break;
                }
                sock_recvn(socket, rte_pktmbuf_mtod(*(v->head + i), void*),
                    block_size);
                to_read = to_read - block_size;
        }
        nb_mbuf = i + 1;
        return nb_mbuf;
}
