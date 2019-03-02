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

#include "ipc.h"

/************************
 *  Unix Domain Socket  *
 ************************/

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

void send_mbufs_data_uds(int socket, struct rte_mbuf** buf, uint16_t size_b,
    uint16_t nb_mbuf, uint16_t tail_size)
{
        uint16_t i;
        uint16_t conv;
        uint8_t* data;

        // Send the total length with 4 bytes
        conv = rte_cpu_to_be_16(size_b);
        data = (uint8_t*)(&conv);
        send(socket, data, 4, 0);
        for (i = 0; i < nb_mbuf - 1; ++i) {
                data = rte_pktmbuf_mtod((*(buf + i)), uint8_t*);
                send(socket, data, (*(buf + i))->data_len, 0);
        }
        data = rte_pktmbuf_mtod((*(buf + nb_mbuf - 1)), uint8_t*);
        send(socket, data, tail_size, 0);
}
