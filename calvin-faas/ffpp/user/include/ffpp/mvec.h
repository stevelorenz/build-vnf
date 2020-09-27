/*
 * ffpp_mvec.h
 */

#ifndef MVEC_H
#define MVEC_H

#include <stdint.h>

#include <rte_atomic.h>
#include <rte_common.h>
#include <rte_mbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 *
 * (WIP) Dynamically resized arrays for pointers of rte_mbufs.
 *
 * One (Main) target use-case of this data structure is used to exchange
 * processing units between processes.
 *
 */

// The same as the maximal burst size in rte_eth_rx_burst().
#define MVEC_INIT_CAPACITY 64

/**
 * Macro to interate over all mbufs in a vector.
 */
#define FFPP_MVEC_FOREACH(vec, i, mbuf)                                        \
	for (i = 0, mbuf = ffpp_mvec_at_index((vec), i);                       \
	     i < ffpp_mvec_len((vec));                                         \
	     i++, mbuf = ffpp_mvec_at_index((vec), i))

/**
 * struct ffpp_mvec - A vector of rte_mbuf pointers.
 *
 * For performance, this data structure should always be allocated on the stack.
 * To operate headers and payload like a Linux-like socket buffer.
 *
 * Socket buffer reference: http://vger.kernel.org/~davem/skb_data.html
 */
struct ffpp_mvec {
	uint16_t len; /**< Number of mbufs in the vector */
	uint16_t capacity; /**< Maximal number of mbufs in the vector */
	uint16_t socket_id;
	struct rte_mbuf **head; /**< Head pointer of a rte_mbuf array */
} __rte_cache_aligned;

/**
 * Initialize the mbuf vector with an array of mbufs.
 *
 * @param vec
 * @param buf: An array of pointers pointing at mbufs.
 * @param size: The size of the array.
 *
 */
int ffpp_mvec_init(struct ffpp_mvec *vec, struct rte_mbuf **buf, uint16_t size);

/**
 * Free the given mbuf vector.
 *
 * @param vec
 */
void ffpp_mvec_free(struct ffpp_mvec *vec);

/**
 * Free all mbufs in the vector (Not the contained mbufs).
 *
 * @param vec
 */
void ffpp_mvec_free_mbufs(struct ffpp_mvec *vec);

/**
 * Get the mbuf with the given index.
 *
 * @param vec
 * @param n
 *
 * @return 
 */
struct rte_mbuf *ffpp_mvec_at_index(struct ffpp_mvec *vec, uint16_t n);

/**
 * @brief Get the current length of the mbuf vector.
 *
 * @param vec
 *
 * @return 
 */
uint16_t ffpp_mvec_len(struct ffpp_mvec *vec);

/**
 * Free a part of mbufs in the vector.
 *
 * @param vec
 * @param offset
 */
void ffpp_mvec_free_mbufs_part(struct ffpp_mvec *vec, uint16_t offset);

/**
 * Print metadata of a mbuf vector.
 *
 * @param vec
 */
void ffpp_mvec_print(const struct ffpp_mvec *vec);

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
int ffpp_mvec_datacmp(struct ffpp_mvec *vec1, struct ffpp_mvec *vec2);

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
void ffpp_mvec_push(struct ffpp_mvec *vec, uint16_t len);

void ffpp_mvec_push_u8(struct ffpp_mvec *vec, uint8_t value);
void ffpp_mvec_push_u16(struct ffpp_mvec *vec, uint16_t value);
void ffpp_mvec_push_u32(struct ffpp_mvec *vec, uint32_t value);
void ffpp_mvec_push_u64(struct ffpp_mvec *vec, uint64_t value);

/**
 * Like skb_pull - Remove data from the beginning of the data room of all mbufs
 * in the vector.
 *
 * @param vec: mbuf vector to modify.
 * @param len: Number of bytes to read from the data room.
 *
 * Raise panic if data room is not enough.
 */
void ffpp_mvec_pull(struct ffpp_mvec *vec, uint16_t len);

void ffpp_mvec_pull_u8(struct ffpp_mvec *vec, uint8_t *values);
void ffpp_mvec_pull_u16(struct ffpp_mvec *vec, uint16_t *values);
void ffpp_mvec_pull_u32(struct ffpp_mvec *vec, uint32_t *values);
void ffpp_mvec_pull_u64(struct ffpp_mvec *vec, uint64_t *values);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !MVEC_H */
