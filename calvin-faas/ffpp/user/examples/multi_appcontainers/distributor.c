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

#define OUT_QUEUE_SIZE 128

#define NUM_MBUFS 5000
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RX_BUF_SIZE 80

static volatile bool force_quit = false;

struct rte_mempool *mbuf_pool;

struct rte_ring *out_queue;

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

	// MARK: Hard-coded for testing
	const unsigned nb_ports = 1;
	printf("%u ports were found\n", nb_ports);
	if (nb_ports != 1)
		rte_exit(EXIT_FAILURE, "Error: ONLY support one port! \n");

	mbuf_pool =
		ffpp_init_mempool("distributor_pool", NUM_MBUFS * nb_ports,
				  rte_socket_id(), RTE_MBUF_DEFAULT_BUF_SIZE);
	out_queue =
		rte_ring_create("distributor_outqueue", OUT_QUEUE_SIZE,
				rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);

	// Init the device with port_id 0
	struct dpdk_device_config cfg = { 0, &mbuf_pool, 1, 1, 256, 256, 1, 1 };
	dpdk_init_device(&cfg);

	if (out_queue == NULL || mbuf_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Problem getting rings or mempool\n");
	}

	print_lcore_infos();

	dpdk_enter_mainloop_master(proc_loop_master, NULL);

	rte_eal_cleanup();
	return 0;
}
