/*
 * memory.h
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <rte_mempool.h>

/**
 * @brief ffpp_init_mempool
 *
 * @param name
 * @param nb_mbuf
 * @param mbuf_size
 * @param socket_id
 *
 * @return
 */
struct rte_mempool *ffpp_init_mempool(const char *name, uint32_t nb_mbuf,
				      uint32_t mbuf_size, uint32_t socket_id);

#endif /* !MEMORY_H */
