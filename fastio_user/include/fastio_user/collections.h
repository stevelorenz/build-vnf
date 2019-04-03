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
        struct rte_mempool* pool;

        uint8_t* m_data; /* Pointer to the start of the payload of each mbuf */
        uint8_t* m_tail; /* Pointer to the end of the payload */

        const volatile void* pre_fetch0; /* To be pre-fetched cache-line before
                                            vector processing */
};

/**
 * @brief Initialize a mbuf vector
 *
 * @param vec
 *
 * @return
 */
int mbuf_vec_init(struct mbuf_vec* vec);

#endif /* !COLLECTIONS_H */
