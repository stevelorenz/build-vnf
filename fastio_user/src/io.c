/*
 * io.c
 * About: DPDK wrappers for frames IO, Try to use the style of Python socket API
 */

#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_config.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eth_ctrl.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_pci.h>
#include <rte_udp.h>

//#define STB_IMAGE_IMPLEMENTATION
///* ISSUE: stb_image.h has warnings for cast-qual and type-limits, since it is
// * just for testing, is temporally ignored */
//#pragma GCC diagnostic push // require GCC 4.6
//#pragma GCC diagnostic ignored "-Wcast-qual"
//#pragma GCC diagnostic ignored "-Wtype-limits"
//#include "stb_image.h"
//#pragma GCC diagnostic pop // require GCC 4.6

#include "io.h"
#include "utils.h"

#define MAGIC_DATA 0x17
#define MBUF_l3_HDR_LEN 20
#define MBUF_l4_UDP_HDR_LEN 8

/*************************
 *  Blocking IO Wrapper  *
 *************************/

void dpdk_recv_into(uint16_t port_id, uint16_t queue_id,
    struct rte_mbuf** rx_buf, uint16_t offset, uint16_t rx_nb)
{
        uint16_t rx_ed_nb = 0;
        while (rx_ed_nb < rx_nb) {
                rx_ed_nb += rte_eth_rx_burst(port_id, queue_id,
                    rx_buf + offset + rx_ed_nb, rx_nb - rx_ed_nb);
                rte_delay_us(1000);
        }
}

/*********************
 *  Mbuf Operations  *
 *********************/

void dpdk_free_buf(struct rte_mbuf** buf, uint16_t buf_size)
{
        uint16_t i = 0;
        for (i = 0; i < buf_size; ++i) {
                rte_pktmbuf_free(*(buf + i));
        }
}

/**************************
 *  Func for local Tests  *
 **************************/

uint16_t gen_rx_buf_from_file(char const* pathname, struct rte_mbuf** rx_buf,
    uint16_t rx_buf_size, struct rte_mempool* pool, uint16_t MTU,
    uint16_t* tail_size)
{
        uint32_t size_b = 0;
        uint16_t nb_mbuf = 0;
        uint16_t i = 0;
        uint8_t* data;
        FILE* fd = NULL;

        fd = fopen(pathname, "r");
        fseek(fd, 0, SEEK_END);
        size_b = ftell(fd);
        fseek(fd, 0, SEEK_SET);
        // Round up q = (x + y - 1) / y
        nb_mbuf = (size_b + MTU - 1) / MTU;
        if (nb_mbuf > rx_buf_size) {
                rte_exit(EXIT_FAILURE,
                    "The size of the rx buffer is not enough to load the "
                    "file! Minimal mbuf number: %d\n",
                    nb_mbuf);
        }
        *tail_size = size_b - ((nb_mbuf - 1) * MTU);

        for (i = 0; i < nb_mbuf; ++i) {
                *(rx_buf) = rte_pktmbuf_alloc(pool);
                if (*(rx_buf) == NULL) {
                        rte_exit(EXIT_FAILURE, "Can not allocate new mbufs\n");
                }
                data = rte_pktmbuf_mtod(*(rx_buf), uint8_t*);
                if (i == nb_mbuf - 1) {
                        (*(rx_buf))->data_len = *tail_size;
                        (*(rx_buf))->pkt_len = *tail_size;
                        if (fread(data, *tail_size, 1, fd) != 1) {
                                rte_exit(EXIT_FAILURE, "Can not read file!\n");
                        }
                } else {
                        (*(rx_buf))->data_len = MTU;
                        (*(rx_buf))->pkt_len = MTU;
                        if (fread(data, MTU, 1, fd) != 1) {
                                rte_exit(EXIT_FAILURE, "Can not read file!\n");
                        }
                }
                rx_buf += 1;
        }

        fclose(fd);

        return nb_mbuf;
}
