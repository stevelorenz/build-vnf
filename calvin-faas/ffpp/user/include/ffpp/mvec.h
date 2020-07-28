/*
 * mvec.h
 */

#ifndef MVEC_H
#define MVEC_H

#include <rte_atomic.h>
#include <rte_common.h>
#include <rte_mbuf.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * Dynamically resized arrays for pointers of rte_mbufs.
 * This data structure is used to exchange processing units between processes.
 *
 */

// The same as the maximal burst size in rte_eth_rx_burst().
#define MVEC_INIT_CAPACITY 32

/**
 * Macro to interate over all mbufs in a vector.
 */
#define MVEC_FOREACH_MBUF(i, mbuf, vec)                                        \
	for (i = 0, mbuf = mvec_at_index((vec), i); i < ffpp_mvec_len((vec));  \
	     i++, mbuf = mvec_at_index((vec), i))

/**
 * struct mvec - DPDK Mbuf vector.
 *
 * For performance, this data structure should always be allocated on the stack.
 * To operate headers and payload like a Linux-like socket buffer.
 *
 * Socket buffer reference: http://vger.kernel.org/~davem/skb_data.html
 */
struct mvec;

/**
 * Create a new mbuf vector.
 *
 * @return: Pointer to the new mbuf vector.
 */
struct mvec *mvec_new(void);

/**
 * Free the given mbuf vector.
 */
void mvec_free(struct mvec *vec);

/**
 * Free all mbufs in the vector.
 *
 * @param vec
 */
void mvec_free_mbufs(struct mvec *vec);

/**
 * Free a part of mbufs in the vector.
 *
 * @param vec
 * @param offset
 */
void mvec_free_mbufs_part(struct mvec *vec, uint16_t offset);

/**
 * Print metadata of a mbuf vector.
 *
 * @param vec
 */
void print_mvec(struct mvec *vec);

/**
 * Compare two mbuf vectors.
 *
 * @param vec1,vec2
 * Pointers to mbuf vector structs
 *
 * @return:
 * -1 if vec1 is smaller than vec2 in uint8_t format. +1 if vec1 is bigger.  0 if vec1
 *  and vec2 compare equal.
 */
int mvec_datacmp(struct mvec *vec1, struct mvec *vec2);

/**
 * Like skb_push. Push data to the reserved header room of all mbufs in the
 * vector. Data is prepended to mbuf data room.
 *
 * @param vec: mbuf vector to modify.
 * @param len: Number of bytes to mark as used.
 *
 * Raise panic if header room is not enough.
 *
 */
void mvec_push(struct mvec *vec, uint16_t len);

void mvec_push_u8(struct mvec *vec, uint8_t value);
void mvec_push_u16(struct mvec *vec, uint16_t value);
void mvec_push_u32(struct mvec *vec, uint32_t value);
void mvec_push_u64(struct mvec *vec, uint64_t value);

/**
 * Like skb_pull - Remove data from the beginning of the data room of all mbufs
 * in the vector.
 *
 * @param vec: mbuf vector to modify.
 * @param len: Number of bytes to read from the data room.
 *
 * Raise panic if data room is not enough.
 */
void mvec_pull(struct mvec *vec, uint16_t len);

void mvec_pull_u8(struct mvec *vec, uint8_t *values);
void mvec_pull_u16(struct mvec *vec, uint16_t *values);
void mvec_pull_u32(struct mvec *vec, uint32_t *values);
void mvec_pull_u64(struct mvec *vec, uint64_t *values);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !MVEC_H */
