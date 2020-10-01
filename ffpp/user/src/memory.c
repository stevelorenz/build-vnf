/*
 * memory.c
 */

#include <rte_common.h>
#include <rte_config.h>
#include <rte_errno.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_spinlock.h>

#include <stdint.h>

#include "memory.h"

#define MEMPOOL_CACHE_SIZE 256

/*
 * MARK: Some NICs need at least 2KB buffer to receive standard Ethernet frame.
 * Minimal buffer length is 2KB + RTE_PKTMBUF_HEADROOM
 * */
struct rte_mempool *ffpp_init_mempool(const char *name, uint32_t nb_mbuf,
				      uint32_t socket, uint32_t mbuf_size)
{
	struct rte_mempool *pool = NULL;
	// rte_mempool_create is not thread-safe.
	static rte_spinlock_t lock = RTE_SPINLOCK_INITIALIZER;
	rte_spinlock_lock(&lock);
	// rte_pktmbuf_pool_create is a wrapper for rte_mempool create function
	// the socket id can be SOCKET_ID_ANT if there is no NUMA constriant for
	// reserved zone.
	pool = rte_pktmbuf_pool_create(name, nb_mbuf, MEMPOOL_CACHE_SIZE, 0,
				       mbuf_size, socket);
	rte_spinlock_unlock(&lock);
	if (pool == NULL) {
		RTE_LOG(EMERG, MEMPOOL, "Failed to init memory pool: %s\n",
			name);
	}

	return pool;
}
