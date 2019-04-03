/*
 * collections.c
 */

#include <rte_mbuf.h>
#include <rte_prefetch.h>

#include "collections.h"

int mbuf_vec_init(struct mbuf_vec* vec)
{
        int ret;
        uint16_t i;

        ret = rte_pktmbuf_alloc_bulk(vec->pool, vec->head, vec->len);
        if (ret != 0) {
                rte_exit(EXIT_FAILURE,
                    "Failed to init a new mbuf vector with id:%u\n", vec->id);
        }
        for (i = 0; i < vec->len; ++i) {
                vec->m_data = rte_pktmbuf_mtod(*(vec->head + i), uint8_t*);
                vec->m_tail = vec->m_data;
        }

        return 0;
}
