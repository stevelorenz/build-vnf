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

#ifdef __cplusplus
extern "C" {
#endif

// These parameters require fine-tuning.
#define FFPP_MUNF_NAME_MAX_LEN 100
#define FFPP_MUNF_RING_NAME_MAX_LEN (FFPP_MUNF_NAME_MAX_LEN + 10)
#define FFPP_MUNF_NB_MBUFS_DEFAULT 5000

#define FFPP_MUNF_RX_RING_SIZE 128
#define FFPP_MUNF_TX_RING_SIZE 128

/**
 * MuNF manager.
 */
struct ffpp_munf_manager {
	char nf_name[FFPP_MUNF_NAME_MAX_LEN];
	uint16_t rx_port_id;
	uint16_t tx_port_id;
};

struct ffpp_munf_data {
	char rx_ring_name[FFPP_MUNF_RX_RING_SIZE];
	char tx_ring_name[FFPP_MUNF_TX_RING_SIZE];
};

/**
 * Initialize the MuNF manager running as a primary process.
 */
int ffpp_munf_init_manager(struct ffpp_munf_manager *manager,
			   const char *nf_name, struct rte_mempool *pool);

/**
 * Cleanup the MuNF manager.
 */
void ffpp_munf_cleanup_manager(struct ffpp_munf_manager *manager);

/**
 * Register a new MuNF at the end of the queue.
 *
 * @param name: The name of the new MuNF.
 * @param data: The data of the new MuNF.
 *
 * @return 
 * - 0 on success.
 * - -1 on failure.
 */
int ffpp_munf_register(const char *name, struct ffpp_munf_data *data);

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !MUNF_H */
