/*
 * nf.c
 */

#include <stdint.h>
#include <stdio.h>

#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_ethdev.h>

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

int ffpp_munf_init_primary(const char *nf_name, struct rte_mempool *pool)
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

	return 0;
}

void ffpp_munf_cleanup_primary(void)
{
	uint16_t port_id = 0;
	for (port_id = 0; port_id < 2; ++port_id) {
		rte_eth_dev_stop(port_id);
		rte_eth_dev_close(port_id);
	}
}
