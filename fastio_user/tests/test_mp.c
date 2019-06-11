/*
 * About: Test multi-process functions
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

#include <fastio_user/collections.h>
#include <fastio_user/device.h>
#include <fastio_user/io.h>
#include <fastio_user/memory.h>
#include <fastio_user/mp.h>
#include <fastio_user/task.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NUM_MBUFS 5000
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RX_BUF_SIZE 80

#define RTE_LOGTYPE_MP RTE_LOGTYPE_USER2

static volatile bool force_quit = false;

struct rte_mempool* mbuf_pool;

/* Use rings to emulate NIC queues */
struct rte_ring *send_ring, *recv_ring;

static void quit_signal_handler(int signum)
{
        if (signum == SIGINT || signum == SIGTERM) {
                printf(
                    "\n\nSignal %d received, preparing to exit...\n", signum);
                force_quit = true;
        }
}

static int proc_loop_prim(__attribute__((unused)) void* dummy)
{
        struct rte_mbuf* rx_buf[RX_BUF_SIZE];
        uint16_t nb_mbuf = 0;
        uint16_t tail_size = 0;
        struct mbuf_vec* vec = NULL;
        void* msg;

        // BUG: The port_id and queue_id should be passed into proc_loop
        // in a proper way.
        /*dpdk_recv_into(0, 0, rx_buf, 0, RX_BUF_SIZE);*/
        nb_mbuf = gen_rx_buf_from_file(
            "./pikachu.jpg", rx_buf, RX_BUF_SIZE, mbuf_pool, 1500, &tail_size);
        RTE_LOG(INFO, MP, "%d mbufs are allocated. Tail size:%d.\n", nb_mbuf,
            tail_size);

        vec = mbuf_vec_init(rx_buf, nb_mbuf);
        print_mbuf_vec(vec);
        mbuf_vec_free(vec);

        if (rte_mempool_get(mbuf_pool, &msg) < 0) {
                rte_panic("Failed to get a message buffer\n");
        }
        snprintf((char*)msg, 128, "%s", "Hello world!");
        while (rte_ring_enqueue(send_ring, &msg) < 0) {
                usleep(1000);
        }
        sleep(10);
        return 0;
}

static int proc_loop_second(__attribute__((unused)) void* dummy)
{
        void* msg;

        RTE_LOG(DEBUG, MP,
            "Enter proc loop of the secondary process, lcore ID:%u\n",
            rte_lcore_id());

        while (!force_quit) {
                printf("Enter dequeue loop\n");
                while (rte_ring_dequeue(recv_ring, &msg) < 0) {
                        usleep(5);
                }
                printf("Secondary proc received: '%s'\n", (char*)msg);
                rte_mempool_put(mbuf_pool, msg);
        }

        return 0;
}

/**
 * @brief main
 *
 */
int main(int argc, char* argv[])
{
        uint16_t port_id;
        uint16_t lcore_id;
        const uint16_t ring_size = 64;

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

        if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
                printf("This is the primary process.\n");
                send_ring = rte_ring_create("PRI2SEC", ring_size,
                    rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
                recv_ring = rte_ring_create("SEC2PRI", ring_size,
                    rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
                mbuf_pool
                    = dpdk_init_mempool("test_mp_pool", NUM_MBUFS * nb_ports,
                        rte_socket_id(), RTE_MBUF_DEFAULT_BUF_SIZE);
                // Init the device with port_id 0
                struct dpdk_device_config cfg
                    = { 0, &mbuf_pool, 1, 1, 256, 256, 1, 1 };
                dpdk_init_device(&cfg);
        } else {
                printf("This is the secondary process.\n");
                recv_ring = rte_ring_lookup("PRI2SEC");
                send_ring = rte_ring_lookup("SEC2PRI");
                mbuf_pool = rte_mempool_lookup("test_mp_pool");
        }

        if (send_ring == NULL || recv_ring == NULL || mbuf_pool == NULL) {
                rte_exit(EXIT_FAILURE, "Problem getting rings or mempool\n");
        }

        print_lcore_infos();

        if (rte_eal_process_type() == RTE_PROC_PRIMARY) {
                dpdk_enter_mainloop_master(proc_loop_prim, NULL);
        } else {
                dpdk_enter_mainloop_master(proc_loop_second, NULL);
        }

        return 0;
}
