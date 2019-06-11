/*
 * collections.c
 */

#include <rte_compat.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_prefetch.h>

#include "collections.h"

struct mbuf_vec* mbuf_vec_init(struct rte_mbuf** rx_buf, uint16_t len)
{
        static volatile uint16_t mbuf_vec_id = 0;
        struct mbuf_vec* vec = (struct mbuf_vec*)(rte_zmalloc(
            "mbuf_vec", sizeof(struct mbuf_vec), 0));

        vec->id = mbuf_vec_id;
        vec->head = rx_buf;
        vec->len = len;

        __sync_fetch_and_add(&mbuf_vec_id, 1);
        return vec;
}

void mbuf_vec_free(struct mbuf_vec* vec)
{
        uint16_t i;
        for (i = 0; i < vec->len; ++i) {
                rte_pktmbuf_free(*(vec->head + i));
        }
        rte_free(vec);
}

void print_mbuf_vec(struct mbuf_vec* vec)
{
        printf("Mbuf vector ID: %u\n", vec->id);
        printf("Vector length: %u\n", vec->len);
}
