/*
 * test.c
 * About: Test ncmbuf
 *
 */

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_per_lcore.h>
#include <rte_random.h>
#include <rte_ring.h>

#include <nckernel.h>
#include <skb.h>

#include "ncmbuf.h"

#define NB_MBUF 128
#define MBUF_DATA_SIZE 2048
#define MBUF_l2_HDR_LEN 14
#define MBUF_l3_HDR_LEN 20
#define MBUF_l4_UDP_HDR_LEN 8

#define MAGIC_DATA 19940117

struct nck_option_value TEST_OPTION[]
    = { { "protocol", "noack" }, { "symbol_size", "1402" }, { "symbols", "2" },
              { "redundancy", "1" }, { NULL, NULL } };

int main(int argc, char** argv)
{
        int ret;

        ret = rte_eal_init(argc, argv);
        if (ret < 0) {
                rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
        }
        argc -= ret;
        argv += ret;

        struct nck_encoder enc;
        /*struct nck_decoder dec;*/
        /*struct nck_recoder rec;*/

        if (nck_create_encoder(
                &enc, NULL, TEST_OPTION, nck_option_from_array)) {
                rte_exit(EXIT_FAILURE, "Failed to create encoder.\n");
        }

        return ret;
}
