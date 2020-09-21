/*
 * nf.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_string_fns.h>

#include <ffpp/config.h>
#include <ffpp/device.h>
#include <ffpp/memory.h>
#include <ffpp/munf.h>

// TODO: Add config file based interface.
int ffpp_munf_eal_init(int argc, char *argv[])
{
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	return ret;
}

static void ffpp_munf_init_primary_port(uint16_t port_id,
					struct rte_mempool *pool)
{
	struct ffpp_dpdk_device_config dev_cfg = { .port_id = port_id,
						   .pool = &pool,
						   .rx_queues = 1,
						   .tx_queues = 1,
						   .rx_descs = 1024,
						   .tx_descs = 1024,
						   .drop_enabled = 1,
						   .disable_offloads = 1 };
	ffpp_dpdk_init_device(&dev_cfg);
}

static int ffpp_munf_init_primary_rings(struct munf_ctx *ctx)
{
	char ring_name[FFPP_MUNF_NAME_MAX_LEN + 10];
	snprintf(ring_name, FFPP_MUNF_NAME_MAX_LEN + 10, "%s%s", ctx->nf_name,
		 "_rx_ring");
	// RX queue is single-producer, multi-consumers (for scaling)
	ctx->rx_ring = rte_ring_create(ring_name, FFPP_MUNF_RX_RING_SIZE,
				       rte_socket_id(), RING_F_SP_ENQ);

	snprintf(ring_name, FFPP_MUNF_NAME_MAX_LEN + 10, "%s%s", ctx->nf_name,
		 "_tx_ring");

	// TX queue is muli-producers, single-consumer.
	ctx->tx_ring = rte_ring_create(ring_name, FFPP_MUNF_TX_RING_SIZE,
				       rte_socket_id(), RING_F_SC_DEQ);

	if (ctx->rx_ring == NULL || ctx->tx_ring == NULL) {
		return -1;
	}

	return 0;
}

int ffpp_munf_init_primary(struct munf_ctx *ctx, const char *nf_name,
			   struct rte_mempool *pool)
{
	// Sanity checks
	if (rte_eal_process_type() != RTE_PROC_PRIMARY) {
		rte_eal_cleanup();
		rte_exit(EXIT_FAILURE,
			 "This function must be called by a primary process.");
	}
	unsigned int nb_lcore = rte_lcore_count();
	if (nb_lcore != 1) {
		rte_exit(
			EXIT_FAILURE,
			"MuNF: Primary process currently only supports single lcore.\n");
	}

	uint16_t nb_ports;
	nb_ports = rte_eth_dev_count_avail();
	RTE_LOG(INFO, FFPP, "MuNF: Avalable ports number: %u\n", nb_ports);
	if (nb_ports != 2) {
		rte_exit(
			EXIT_FAILURE,
			"Each MuNF must have TWO avaiable ports (Used as ingress and egress ports)\n");
	}

	ctx->rx_port_id = 0;
	ctx->tx_port_id = 1;
	rte_strscpy(ctx->nf_name, nf_name, FFPP_MUNF_NAME_MAX_LEN);

	// Init memory pool.
	RTE_LOG(INFO, FFPP,
		"MuNF: Init the memory pool for network function: %s\n",
		nf_name);
	pool = ffpp_init_mempool(nf_name, FFPP_MUNF_NB_MBUFS_DEFAULT * nb_ports,
				 rte_socket_id(), RTE_MBUF_DEFAULT_BUF_SIZE);
	if (pool == NULL) {
		rte_exit(
			EXIT_FAILURE,
			"Faild to init the memory pool for network function: %s\n",
			nf_name);
	}

	// Init ingress and egress ports (eth devices).
	uint16_t port_id = 0;
	RTE_ETH_FOREACH_DEV(port_id)
	{
		ffpp_munf_init_primary_port(port_id, pool);
	}

	if (ffpp_munf_init_primary_rings(ctx) == -1) {
		rte_exit(EXIT_FAILURE,
			 "MuNF: Failed to create RX and TX rings\n");
	}

	return 0;
}

void ffpp_munf_cleanup_primary(struct munf_ctx *ctx)
{
	rte_ring_free(ctx->rx_ring);
	rte_ring_free(ctx->tx_ring);

	rte_eth_dev_stop(ctx->rx_port_id);
	rte_eth_dev_close(ctx->rx_port_id);
	rte_eth_dev_stop(ctx->tx_port_id);
	rte_eth_dev_close(ctx->tx_port_id);

	rte_eal_cleanup();
}
