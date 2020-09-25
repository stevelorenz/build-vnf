/*
 * nf.h
 */

/**
 * @file
 *
 * Micro Network Function (MuNF) related APIs.
 *
 */

#ifndef NF_H
#define NF_H

#include <stdint.h>

#include <rte_ring.h>

// These parameters require fine-tuning.
#define FFPP_MUNF_NAME_MAX_LEN 100
#define FFPP_MUNF_NB_MBUFS_DEFAULT 5000

#define FFPP_MUNF_RX_RING_SIZE 128
#define FFPP_MUNF_TX_RING_SIZE 128

/**
 * Meta data of a MuNF.
 */
struct munf_ctx_t {
	char nf_name[FFPP_MUNF_NAME_MAX_LEN];
	uint16_t rx_port_id;
	uint16_t tx_port_id;
	struct rte_ring *rx_ring;
	struct rte_ring *tx_ring;
};

/**
 * Initialize the EAL for a MuNF.
 */
int ffpp_munf_eal_init(int argc, char *argv[]);

/**
 * Initialize the setup for a MuNF running as a primary process.
 *
 * @param ctx: The context of type struct munf_ctx_t.
 * @param nf_name: The name of the the network function.
 * @param pool: rte_mempool.
 *
 * @return 
 */
int ffpp_munf_init_primary(struct munf_ctx_t *ctx, const char *nf_name,
			   struct rte_mempool *pool);

void ffpp_munf_cleanup_primary(struct munf_ctx_t *ctx);

#endif /* !NF_H */
