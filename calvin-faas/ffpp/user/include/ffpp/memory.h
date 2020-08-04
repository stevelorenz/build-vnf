/*
 * memory.h
 */

#ifndef MEMORY_H
#define MEMORY_H

/**
 * @brief ffpp_init_mempool
 *
 * @param name
 * @param nb_mbuf
 * @param socket
 * @param mbuf_size
 *
 * @return
 */
struct rte_mempool *ffpp_init_mempool(const char *name, uint32_t nb_mbuf,
				      uint32_t socket, uint32_t mbuf_size);

#endif /* !MEMORY_H */
