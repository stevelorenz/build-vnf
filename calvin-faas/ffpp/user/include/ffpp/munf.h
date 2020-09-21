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
struct munf_ctx {
	char nf_name[FFPP_MUNF_NAME_MAX_LEN];
	uint16_t rx_port_id;
	uint16_t tx_port_id;
	struct rte_ring *rx_ring;
	struct rte_ring *tx_ring;
};

// TODO: Add docs.
int ffpp_munf_eal_init(int argc, char *argv[]);

int ffpp_munf_init_primary(struct munf_ctx *ctx, const char *nf_name,
			   struct rte_mempool *pool);
void ffpp_munf_cleanup_primary(struct munf_ctx *ctx);

#endif /* !NF_H */
