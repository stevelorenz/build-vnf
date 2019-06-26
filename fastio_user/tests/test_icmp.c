/*
 * About: Handle only ICMP flow and response Ping.
 */

#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <rte_cycles.h>
#include <rte_eal.h>

#include <fastio_user/collections.h>
#include <fastio_user/config.h>
#include <fastio_user/device.h>
#include <fastio_user/io.h>
#include <fastio_user/memory.h>
#include <fastio_user/task.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NUM_MBUFS 5000
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RX_BUF_SIZE 80

static volatile bool force_quit = false;

struct rte_mempool* mbuf_pool;

static void quit_signal_handler(int signum)
{
        if (signum == SIGINT || signum == SIGTERM) {
                printf(
                    "\n\nSignal %d received, preparing to exit...\n", signum);
                force_quit = true;
        }
}

static int proc_loop(__attribute__((unused)) void* dummy)
{
        struct rte_mbuf* rx_buf[RX_BUF_SIZE];
        uint16_t pkt_num = 3;
        uint16_t i;

        // BUG: The port_id and queue_id should be passed into proc_loop
        // in a proper way.
        for (i = 0; i < pkt_num; ++i) {
                dpdk_recv_into(0, 0, rx_buf, 0, 1);
        }
        return 0;
}

int main(int argc, char* argv[])
{
        uint16_t port_id;

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

        mbuf_pool = dpdk_init_mempool("test_mbuf_pool", NUM_MBUFS * nb_ports,
            rte_socket_id(), RTE_MBUF_DEFAULT_BUF_SIZE);

        // Init the device with port_id 0
        struct dpdk_device_config cfg = { 0, &mbuf_pool, 1, 1, 256, 256, 1, 1 };
        dpdk_init_device(&cfg);

        dpdk_enter_mainloop_master(proc_loop, NULL);

        rte_eal_cleanup();
        return 0;
}
