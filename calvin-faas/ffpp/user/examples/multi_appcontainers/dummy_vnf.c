/*
 * About:
 */
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_ring.h>

#include <ffpp/collections.h>
#include <ffpp/config.h>
#include <ffpp/device.h>
#include <ffpp/io.h>
#include <ffpp/memory.h>
#include <ffpp/task.h>

#define IN_QUE_SIZE 128
#define OUT_QUE_SIZE 128

#define NUM_MBUFS 5000
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RX_BUF_SIZE 80

static volatile bool force_quit = false;

struct rte_mempool *mbuf_pool;

struct rte_ring *in_queue;

static void quit_signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
		       signum);
		force_quit = true;
	}
}

static int proc_loop_master(__attribute__((unused)) void *dummy)
{
	while (!force_quit) {
		rte_delay_ms(500);
	}

	return 0;
}

/**
 * @brief main
 *
 */
int main(int argc, char *argv[])
{
	uint16_t port_id;
	uint16_t lcore_id;

	// Init EAL environment
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	signal(SIGINT, quit_signal_handler);
	signal(SIGTERM, quit_signal_handler);

	if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
		rte_eal_cleanup();
		rte_exit(EXIT_FAILURE,
			 "Dummpy VNF must run as secondary process.\n");
	}

	// MARK: Hard-coded for testing
	const unsigned nb_ports = 1;
	printf("%u ports were found\n", nb_ports);
	if (nb_ports != 1)
		rte_exit(EXIT_FAILURE, "Error: ONLY support one port! \n");

	/* Get already allocated memory pool */
	mbuf_pool = rte_mempool_lookup("distributor_pool");
	if (mbuf_pool == NULL) {
		rte_eal_cleanup();
		rte_exit(EXIT_FAILURE, "Can not find distributor_pool\n");
	}

	in_queue = rte_ring_lookup("distributor_outqueue");
	if (in_queue == NULL) {
		rte_eal_cleanup();
		rte_exit(EXIT_FAILURE, "Can not find distributor_outqueue\n");
	}

	print_lcore_infos();

	dpdk_enter_mainloop_master(proc_loop_master, NULL);

	rte_eal_cleanup();
	return 0;
}
