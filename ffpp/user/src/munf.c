/*
 * munf.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_errno.h>

#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_string_fns.h>
#include <rte_tailq.h>

#include <ffpp/config.h>
#include <ffpp/device.h>
#include <ffpp/memory.h>
#include <ffpp/munf.h>

struct munf_entry {
	TAILQ_ENTRY(munf_entry) next;
	char munf_name[FFPP_MUNF_NAME_MAX_LEN];
	char rx_ring_name[FFPP_MUNF_RING_NAME_MAX_LEN];
	char tx_ring_name[FFPP_MUNF_RING_NAME_MAX_LEN];
	// Private fields.
	struct rte_ring *_rx_ring;
	struct rte_ring *_tx_ring;
};

// Double linked list of munf entries
TAILQ_HEAD(munf_entry_list, munf_entry);
static struct munf_entry_list munf_entry_list =
	TAILQ_HEAD_INITIALIZER(munf_entry_list);

static void ffpp_munf_init_manager_port(uint16_t port_id,
					struct rte_mempool *pool)
{
	// MARK: Device parameters are HARD-CODED here.
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

int ffpp_munf_init_manager(struct ffpp_munf_manager *manager,
			   const char *nf_name, struct rte_mempool *pool)
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
	if (nb_ports == 0 || nb_ports > 2) {
		rte_exit(
			EXIT_FAILURE,
			"Each MuNF must have one or two avaiable ports (Used as ingress and egress ports)\n");
	}

	if (nb_ports == 1) {
		manager->rx_port_id = 0;
		manager->tx_port_id = 0;
	} else if (nb_ports == 2) {
		manager->rx_port_id = 0;
		manager->tx_port_id = 1;
	}

	rte_strscpy(manager->nf_name, nf_name, FFPP_MUNF_NAME_MAX_LEN);

	// Init memory pool.
	RTE_LOG(INFO, FFPP,
		"MuNF: Init the memory pool for network function: %s\n",
		nf_name);
	pool = ffpp_init_mempool(nf_name, FFPP_MUNF_NB_MBUFS_DEFAULT * nb_ports,
				 RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (pool == NULL) {
		rte_exit(
			EXIT_FAILURE,
			"Faild to init the memory pool for network function: %s\n",
			nf_name);
	}
	manager->pool = pool;

	// Init ingress and egress ports (eth devices).
	uint16_t port_id = 0;
	RTE_ETH_FOREACH_DEV(port_id)
	{
		ffpp_munf_init_manager_port(port_id, pool);
	}

	return 0;
}

void ffpp_munf_cleanup_manager(struct ffpp_munf_manager *manager)
{
	// Cleanup the MuNF queue.
	struct munf_entry *entry;
	while (!TAILQ_EMPTY(&munf_entry_list)) {
		entry = TAILQ_FIRST(&munf_entry_list);
		rte_ring_free(entry->_rx_ring);
		rte_ring_free(entry->_tx_ring);
		TAILQ_REMOVE(&munf_entry_list, entry, next);
		free(entry);
	}

	rte_eth_dev_stop(manager->rx_port_id);
	rte_eth_dev_close(manager->rx_port_id);
	rte_eth_dev_stop(manager->tx_port_id);
	rte_eth_dev_close(manager->tx_port_id);

	rte_mempool_free(manager->pool);
}

static int munf_register_init_rings(struct munf_entry *entry,
				    struct ffpp_munf_data *data)
{
	snprintf(entry->rx_ring_name, FFPP_MUNF_RING_NAME_MAX_LEN, "%s%s",
		 entry->munf_name, "_rx_ring");
	snprintf(data->rx_ring_name, FFPP_MUNF_RING_NAME_MAX_LEN, "%s%s",
		 entry->munf_name, "_rx_ring");
	entry->_rx_ring =
		rte_ring_create(entry->rx_ring_name, FFPP_MUNF_RX_RING_SIZE,
				rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

	snprintf(entry->tx_ring_name, FFPP_MUNF_RING_NAME_MAX_LEN, "%s%s",
		 entry->munf_name, "_tx_ring");
	snprintf(data->tx_ring_name, FFPP_MUNF_RING_NAME_MAX_LEN, "%s%s",
		 entry->munf_name, "_tx_ring");

	entry->_tx_ring =
		rte_ring_create(entry->tx_ring_name, FFPP_MUNF_TX_RING_SIZE,
				rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

	if (entry->_rx_ring == NULL || entry->_tx_ring == NULL) {
		return -1;
	}

	return 0;
}

static inline struct munf_entry *find_munf_entry_by_name(const char *name)
{
	struct munf_entry *entry;
	TAILQ_FOREACH(entry, &munf_entry_list, next)
	{
		if (strncmp(entry->munf_name, name, FFPP_MUNF_NAME_MAX_LEN) ==
		    0) {
			break;
		}
	}
	return entry;
}

int ffpp_munf_register(const char *name, struct ffpp_munf_data *data)
{
	struct munf_entry *entry;
	entry = malloc(sizeof(struct munf_entry));
	if (entry == NULL) {
		rte_errno = ENOMEM;
		return -1;
	}
	strlcpy(entry->munf_name, name, sizeof(entry->munf_name));
	if (munf_register_init_rings(entry, data) < 0) {
		RTE_LOG(ERR, FFPP, "Failed to create rings for the MuNF: %s\n",
			entry->munf_name);
		return -1;
	}
	TAILQ_INSERT_TAIL(&munf_entry_list, entry, next);

	return 0;
}

int ffpp_munf_unregister(const char *name)
{
	struct munf_entry *entry;
	entry = find_munf_entry_by_name(name);
	if (entry == NULL) {
		rte_errno = -EINVAL;
		return -1;
	}
	TAILQ_REMOVE(&munf_entry_list, entry, next);
	free(entry);
	return 0;
}
