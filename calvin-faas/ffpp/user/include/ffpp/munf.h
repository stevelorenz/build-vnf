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

#include <rte_ethdev.h>

// These parameters require fine-tuning.
#define FFPP_MUNF_NAME_MAX_LEN 100
#define FFPP_MUNF_NB_MBUFS_DEFAULT 5000

#define FFPP_MUNF_RX_RING_SIZE 128
#define FFPP_MUNF_TX_RING_SIZE 128

/**
 * Meta data of a MuNF.
 */
struct munf_meta {
	char nf_name[FFPP_MUNF_NAME_MAX_LEN];
	char port_in_name[RTE_ETH_NAME_MAX_LEN];
	char port_out_name[RTE_ETH_NAME_MAX_LEN];
};

// TODO: Add docs.
int ffpp_munf_eal_init(int argc, char *argv[]);

int ffpp_munf_init_primary(const char *nf_name, struct rte_mempool *pool);
void ffpp_munf_cleanup_primary(void);

#endif /* !NF_H */
