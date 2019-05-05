/*
 * About: Test DPDK ring functions
 *
 *        Can be used as a emulation for single-producer and -consumer FIFO
 *        queue based pktgen --- VNF. Namely, master core -> consumer and VNF,
 *        slave core -> producer and pktgen.
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
#include <fastio_user/task.h>

#define IN_QUE_SIZE 128
#define OUT_QUE_SIZE 128

#define NUM_MBUFS 5000
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RX_BUF_SIZE 40

static volatile bool force_quit = false;

struct rte_mempool* mbuf_pool;

/* Use rings to emulate NIC queues */
struct rte_ring *in_que, *out_que;

static void quit_signal_handler(int signum)
{
        if (signum == SIGINT || signum == SIGTERM) {
                printf(
                    "\n\nSignal %d received, preparing to exit...\n", signum);
                force_quit = true;
        }
}

static int proc_loop_master(__attribute__((unused)) void* dummy)
{
        struct rte_mbuf* rx_buf[RX_BUF_SIZE];
        void* msg = NULL;
        uint16_t nb_mbuf = 0;
        uint16_t i = 0;
        struct mbuf_vec* vec = NULL;

        /* Dequeue metadata packet */
        while (rte_ring_dequeue(in_que, &msg) < 0) {
                usleep(50);
        }
        nb_mbuf = *((uint16_t*)msg);
        RTE_LOG(
            INFO, RING, "[MASTER] %d mbufs are in the in_queue.\n", nb_mbuf);
        rte_mempool_put(mbuf_pool, msg);

        /* Dequeue data packets */
        for (i = 0; i < nb_mbuf; ++i) {
                rte_ring_dequeue(in_que, &msg);
                *(rx_buf + i) = (struct rte_mbuf*)(msg);
        }
        vec = mbuf_vec_init(rx_buf, nb_mbuf, 0);

        print_mbuf_vec(vec);
        // MARK: Add the processing code of mbuf vectors here, e.g. parse
        // headers and send the payload to another process via Unix domain
        // socket.

        RTE_LOG(INFO, RING, "Free the mbuf vector.\n");
        mbuf_vec_free(vec);
        return 0;
}

static int proc_loop_slave(__attribute__((unused)) void* dummy)
{
        struct rte_mbuf* tx_buf[RX_BUF_SIZE];
        uint16_t i;
        uint16_t nb_mbuf = 0;
        uint16_t ring_size = 0;
        uint16_t free_space = 0;
        uint16_t tail_size = 0;
        void* msg = NULL;

        nb_mbuf = gen_rx_buf_from_file("/dataset/pedestrian_walking/0.jpg",
            tx_buf, RX_BUF_SIZE, mbuf_pool, 1500, &tail_size);

        RTE_LOG(INFO, RING, "[SLAVE] %d mbufs are allocated. Tail size:%d.\n",
            nb_mbuf, tail_size);

        /* Add metadata packet */
        if (rte_mempool_get(mbuf_pool, &msg) < 0)
                rte_panic("Failed to get message buffer\n");
        *(uint16_t*)(msg) = nb_mbuf;

        /* Enqueue data packets */
        rte_ring_enqueue(in_que, msg);
        for (i = 0; i < nb_mbuf; ++i) {
                rte_ring_enqueue(in_que, *(tx_buf + i));
        }

        ring_size = rte_ring_get_size(in_que);
        free_space = rte_ring_free_count(in_que);
        RTE_LOG(INFO, RING,
            "[SLAVE] Enqueue finished, in_que size:%u, free space of "
            "in_que:%u\n",
            ring_size, free_space);

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

        in_que = rte_ring_create("IN_QUE", IN_QUE_SIZE, rte_socket_id(),
            RING_F_SP_ENQ | RING_F_SC_DEQ);
        out_que = rte_ring_create("OUT_QUE", OUT_QUE_SIZE, rte_socket_id(),
            RING_F_SP_ENQ | RING_F_SC_DEQ);
        mbuf_pool = dpdk_init_mempool("test_ring_pool", NUM_MBUFS * nb_ports,
            rte_socket_id(), RTE_MBUF_DEFAULT_BUF_SIZE);

        // Init the device with port_id 0
        struct dpdk_device_config cfg = { 0, &mbuf_pool, 1, 1, 256, 256, 1, 1 };
        dpdk_init_device(&cfg);

        if (in_que == NULL || out_que == NULL || mbuf_pool == NULL) {
                rte_exit(EXIT_FAILURE, "Problem getting rings or mempool\n");
        }

        print_lcore_infos();

        RTE_LOG(INFO, RING, "Launch task on the first slave core.\n");
        RTE_LCORE_FOREACH_SLAVE(lcore_id)
        {
                rte_eal_remote_launch(proc_loop_slave, NULL, lcore_id);
                break;
        }

        dpdk_enter_mainloop_master(proc_loop_master, NULL);

        rte_eal_cleanup();
        return 0;
}
