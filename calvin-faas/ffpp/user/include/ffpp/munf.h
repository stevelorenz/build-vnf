/*
 * munf.h
 */

/**
 * @file
 *
 * Micro Network Function (MuNF) related APIs.
 *
 */

#ifndef MUNF_H
#define MUNF_H

#include <stdint.h>

// These parameters require fine-tuning.
#define FFPP_MUNF_NAME_MAX_LEN 100
#define FFPP_MUNF_RING_NAME_MAX_LEN (FFPP_MUNF_NAME_MAX_LEN + 10)
#define FFPP_MUNF_NB_MBUFS_DEFAULT 5000

#define FFPP_MUNF_RX_RING_SIZE 128
#define FFPP_MUNF_TX_RING_SIZE 128

/**
 * Context meta data of the MuNF manager.
 */
struct ffpp_munf_manager_ctx {
	char nf_name[FFPP_MUNF_NAME_MAX_LEN];
	uint16_t rx_port_id;
	uint16_t tx_port_id;
};

/**
 * Initialize the MuNF manager running as a primary process.
 *
 * @param ctx: The context of type struct ffpp_munf_manager_ctx.
 * @param nf_name: The name of the the network function.
 * @param pool: rte_mempool.
 *
 * @return 
 */
int ffpp_munf_init_manager(struct ffpp_munf_manager_ctx *ctx,
			   const char *nf_name, struct rte_mempool *pool);

/**
 * Cleanup the MuNF manager.
 */
void ffpp_munf_cleanup_manager(struct ffpp_munf_manager_ctx *ctx);

/**
 * Register a new MuNF at the end of the queue.
 *
 * @param name: The name of the new MuNF.
 *
 * @return 
 * - 0 on success.
 * - -1 on failure.
 */
int ffpp_munf_register(const char *name);

/**
 * UnRegister a new MuNF at the end of the queue.
 *
 * @param name: The name of the MuNF.
 *
 * @return 
 * - 0 on success.
 * - -1 on failure.
 */
int ffpp_munf_unregister(const char *name);

#endif /* !MUNF_H */
