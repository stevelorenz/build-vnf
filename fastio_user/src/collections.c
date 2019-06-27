/*
 * collections.c
 */

#include <rte_byteorder.h>
#include <rte_common.h>
#include <rte_compat.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>

#include "collections.h"
#include "config.h"
#include "utils.h"

struct mvec* mvec_new(struct rte_mbuf** mbuf_arr, uint16_t len)
{
        static volatile uint16_t mvec_id = 0;
        struct mvec* vec
            = (struct mvec*)(rte_zmalloc("mvec", sizeof(struct mvec), 0));

        vec->id = mvec_id;
        vec->head = mbuf_arr;
        vec->tail = mbuf_arr + len;
        vec->len = len;
        vec->mbuf_payload_off = RTE_PKTMBUF_HEADROOM;

        __sync_fetch_and_add(&mvec_id, 1);
        return vec;
}

void mvec_free_part(struct mvec* vec, uint16_t offset)
{
        uint16_t i = 0;

        for (i = offset; i < vec->len; ++i) {
                rte_pktmbuf_free(*(vec->head + i));
        }
        rte_free(vec);
}

void mvec_free(struct mvec* vec) { mvec_free_part(vec, 0); }

void print_mvec(struct mvec* vec)
{
        printf("Mbuf vector ID: %u\n", vec->id);
        printf("Vector length: %u\n", vec->len);
        printf("Mbuf head offset: %d\n", rte_pktmbuf_headroom(*(vec->head)));
        printf("Mbuf data offset: %d\n", vec->mbuf_payload_off);
}

int mvec_datacmp(struct mvec* v1, struct mvec* v2)
{
        uint16_t len = 0;
        uint16_t i = 0;

        len = RTE_MIN(v1->len, v2->len);
        for (i = 0; i < len; ++i) {
                int ret = 0;
                ret = mbuf_datacmp(*(v1->head + i), *(v2->head + i));
                if (ret != 0) {
                        return ret;
                }
        }
        return 0;
}

void mvec_push(struct mvec* v, uint16_t len)
{
        uint16_t i = 0;
        char* ret = NULL;

        MVEC_FOREACH_MBUF(i, v)
        {
                ret = rte_pktmbuf_prepend(*(v->head + i), len);
                if (ret == NULL) {
                        rte_panic(
                            "Header room of %d-th mbuf is not enough.\n", i);
                }
        }
}

void mvec_push_u8(struct mvec* v, uint8_t value)
{
        uint16_t i = 0;

        mvec_push(v, sizeof(value));
        MVEC_FOREACH_MBUF(i, v)
        {
                rte_prefetch_non_temporal(
                    rte_pktmbuf_mtod(*(v->head + i), void*));
                rte_memcpy(rte_pktmbuf_mtod(*(v->head + i), uint8_t*), &value,
                    sizeof(value));
        }
}

void mvec_push_u16(struct mvec* v, uint16_t value)
{
        uint16_t i = 0;

        mvec_push(v, sizeof(value));
        value = rte_cpu_to_be_16(value);
        MVEC_FOREACH_MBUF(i, v)
        {
                rte_prefetch_non_temporal(
                    rte_pktmbuf_mtod(*(v->head + i), void*));
                rte_memcpy(rte_pktmbuf_mtod(*(v->head + i), uint16_t*), &value,
                    sizeof(value));
        }
}

void mvec_push_u32(struct mvec* v, uint32_t value)
{
        uint16_t i = 0;

        mvec_push(v, sizeof(value));
        value = rte_cpu_to_be_32(value);
        MVEC_FOREACH_MBUF(i, v)
        {
                rte_prefetch_non_temporal(
                    rte_pktmbuf_mtod(*(v->head + i), void*));
                rte_memcpy(rte_pktmbuf_mtod(*(v->head + i), uint32_t*), &value,
                    sizeof(value));
        }
}

void mvec_push_u64(struct mvec* v, uint64_t value)
{
        uint16_t i = 0;

        mvec_push(v, sizeof(value));
        value = rte_cpu_to_be_64(value);
        MVEC_FOREACH_MBUF(i, v)
        {
                rte_prefetch_non_temporal(
                    rte_pktmbuf_mtod(*(v->head + i), void*));
                rte_memcpy(rte_pktmbuf_mtod(*(v->head + i), uint64_t*), &value,
                    sizeof(value));
        }
}

void mvec_pull(struct mvec* v, uint16_t len)
{
        uint16_t i;
        char* ret = NULL;

        MVEC_FOREACH_MBUF(i, v)
        {
                ret = rte_pktmbuf_adj(*(v->head + i), len);
                if (ret == NULL) {
                        rte_panic(
                            "Data room of %d-th mbuf is not enough.\n", i);
                }
        }
}

void mvec_pull_u8(struct mvec* v, uint8_t* values)
{
        uint16_t i;
        MVEC_FOREACH_MBUF(i, v)
        {
                *(values + i) = *(rte_pktmbuf_mtod(*(v->head + i), uint8_t*));
        }
        mvec_pull(v, sizeof(uint8_t));
}

void mvec_pull_u16(struct mvec* v, uint16_t* values)
{
        uint16_t i;
        MVEC_FOREACH_MBUF(i, v)
        {
                rte_prefetch0(rte_pktmbuf_mtod(*(v->head + i), void*));
                rte_memcpy(values + i,
                    rte_pktmbuf_mtod(*(v->head + i), uint16_t*),
                    sizeof(uint16_t));
                *(values + i) = rte_be_to_cpu_16(*(values + i));
        }
        mvec_pull(v, sizeof(uint16_t));
}

void mvec_pull_u32(struct mvec* v, uint32_t* values)
{
        uint16_t i;
        MVEC_FOREACH_MBUF(i, v)
        {
                rte_prefetch0(rte_pktmbuf_mtod(*(v->head + i), void*));
                rte_memcpy(values + i,
                    rte_pktmbuf_mtod(*(v->head + i), uint32_t*),
                    sizeof(uint32_t));
                *(values + i) = rte_be_to_cpu_32(*(values + i));
        }
        mvec_pull(v, sizeof(uint32_t));
}

void mvec_pull_u64(struct mvec* v, uint64_t* values)
{
        uint16_t i;
        MVEC_FOREACH_MBUF(i, v)
        {
                rte_prefetch0(rte_pktmbuf_mtod(*(v->head + i), void*));
                rte_memcpy(values + i,
                    rte_pktmbuf_mtod(*(v->head + i), uint64_t*),
                    sizeof(uint64_t));
                *(values + i) = rte_be_to_cpu_64(*(values + i));
        }
        mvec_pull(v, sizeof(uint64_t));
}
