/*
 * collections.h
 *
 * Container datatype providing convenient encapsulations and operations on
 * DPDK's general built-in packet container: mbuf
 * (https://doc.dpdk.org/guides/prog_guide/mbuf_lib.html)
 *
 */

#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include <rte_common.h>
#include <rte_mbuf.h>

#include <stdint.h>

#define MBUF_VEC_FOREACH_MBUF(i, vec)

/**
 * struct mbuf_vec - DPDK Mbuf vector
 */
struct mbuf_vec {
        uint16_t id;
        struct rte_mbuf** head; /* Head of a rte_mbuf* array */
        uint16_t len;
        const volatile void* pre_fetch0; /* To be pre-fetched cache-line before
                                            vector processing */
};

/**
 * mbuf_vec_init() - Initialize a mbuf vector
 *
 * @param rx_buf
 * @param len
 *
 * @return
 */
struct mbuf_vec* mbuf_vec_init(struct rte_mbuf** rx_buf, uint16_t len);

/**
 * mbuf_vec_free() - Free all mbufs in the vector
 *
 * @param vec
 */
void mbuf_vec_free(struct mbuf_vec* vec);

/**
 * print_mbuf_vec() - Print metadata of a mbuf vector
 *
 * @param vec
 */
void print_mbuf_vec(struct mbuf_vec* vec);

#endif /* !COLLECTIONS_H */
