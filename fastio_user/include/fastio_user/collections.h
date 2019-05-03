/*
 * collections.h
 *
 * Container datatypes providing convenient encapsulations and operations on
 * DPDK's general built-in packet container: mbuf
 * (https://doc.dpdk.org/guides/prog_guide/mbuf_lib.html)
 *
 */

#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include <rte_common.h>
#include <rte_mbuf.h>

#include <stdint.h>

/**
 * @brief DPDK Mbuf vector
 */
struct mbuf_vec {
        uint16_t id;
        struct rte_mbuf** head; /* Head of a rte_mbuf* array */
        uint16_t len;
        uint16_t tail_size;
        uint16_t* m_payload_head_arr;
        const volatile void* pre_fetch0; /* To be pre-fetched cache-line before
                                            vector processing */
};

struct mbuf_vec* mbuf_vec_init(
    struct rte_mbuf** rx_buf, uint16_t len, uint16_t tail_size);

void mbuf_vec_free(struct mbuf_vec* vec);

void print_mbuf_vec(struct mbuf_vec* vec);

#endif /* !COLLECTIONS_H */
