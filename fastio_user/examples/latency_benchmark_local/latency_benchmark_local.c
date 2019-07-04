/*
 * About: Simple example of benchmark mbuf vector processing latency locally.
 *
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
#include <rte_random.h>
#include <rte_ring.h>

#include <fastio_user/collections.h>
#include <fastio_user/config.h>
#include <fastio_user/device.h>
#include <fastio_user/io.h>
#include <fastio_user/ipc.h>
#include <fastio_user/memory.h>
#include <fastio_user/task.h>
#include <fastio_user/utils.h>

#define NUM_MBUFS 5000
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RT_BUF_SIZE 80

/* Must be power of 2 */
#define L2V_QUE_SIZE 128
#define V2L_QUE_SIZE 128

#define DEQUEUE_SLEEP 50  // us
#define IPC_WAIT_SLEEP 50 // us

#define IMAGE_FRAME_NUM 5
#define RUN_IMAGE_PROC 1

#define TEST_ROUND 5
#define TEST_SLEEP 500 // ms

static volatile bool force_quit = false;

struct rte_mempool* mbuf_pool;

/* Use rings to emulate NIC queues */
struct rte_ring *l2v_que, *v2l_que;

static void quit_signal_handler(int signum)
{
        if (signum == SIGINT || signum == SIGTERM) {
                printf(
                    "\n\nSignal %d received, preparing to exit...\n", signum);
                force_quit = true;
        }
}

void check_pull_values(uint64_t* arr, uint16_t len)
{
        uint16_t i;
        for (i = 0; i < len; ++i) {
                if (arr[i] != FFBB_MAGIC) {
                        rte_panic("ERROR! Pulled values are wrong!");
                }
        }
}

void fake_processing(struct mvec* v)
{
        uint16_t i;
        uint8_t tmp_u8[v->len];
        uint16_t tmp_u16[v->len];
        uint32_t tmp_u32[v->len];
        uint64_t tmp_u64[v->len];

        mvec_push_u8(v, FFBB_MAGIC);
        mvec_pull_u8(v, tmp_u8);
        mvec_push_u16(v, FFBB_MAGIC);
        mvec_pull_u16(v, tmp_u16);
        mvec_push_u32(v, FFBB_MAGIC);
        mvec_pull_u32(v, tmp_u32);
        mvec_push_u64(v, FFBB_MAGIC);
        mvec_pull_u64(v, tmp_u64);
        check_pull_values(tmp_u64, v->len);
        rte_delay_ms(FFBB_MAGIC + rte_rand() % FFBB_MAGIC);
}

/**
 * Main loop of the slave core
 */
static int proc_loop_slave(__attribute__((unused)) void* dummy)
{
        struct rte_mbuf* rt_buf[RT_BUF_SIZE];
        struct mvec* rt_vec = NULL;
        void* meta_data = NULL;
        uint16_t nb_mbuf = 0;
        uint16_t i = 0;
        uint16_t r = 0;
        uint16_t img_idx = 0;
        uint16_t tail_size = 0;
        int ret = 0;
        uint64_t last_tsc = 0;

        int socket = 0;
        socket = init_uds_stream_cli("/uds_socket");

        for (r = 0; r < TEST_ROUND; r++) {
                rte_srand(rte_get_tsc_cycles());
                for (img_idx = 0; img_idx < IMAGE_FRAME_NUM; img_idx++) {
                        while (rte_ring_dequeue(l2v_que, &meta_data) < 0) {
                                rte_delay_us_block(DEQUEUE_SLEEP);
                        }
                        nb_mbuf = *((uint16_t*)meta_data);
                        RTE_LOG(DEBUG, TEST,
                            "[MASTER] %d mbufs are in the l2v_que.\n", nb_mbuf);
                        rte_mempool_put(mbuf_pool, meta_data);
                        rte_ring_dequeue_bulk(
                            l2v_que, (void**)rt_buf, nb_mbuf, NULL);
                        rt_vec = mvec_new(rt_buf, nb_mbuf);
                        /* ts: receive all mbufs */
                        last_tsc = rte_get_tsc_cycles();
                        if (likely(RUN_IMAGE_PROC == 1)) {
                                // fake_processing(rt_vec);
                                send_mvec_data_uds(socket, rt_vec);
                                /* Get processed and re-packetized data */
                                nb_mbuf = recv_mvec_data_uds(socket, rt_vec);
                        }
                        /* Push the processing time, including IPC and
                         * processing time
                         */
                        mvec_push_u64(rt_vec, rte_get_tsc_cycles() - last_tsc);

                        mvec_free_part(rt_vec, nb_mbuf);
                        /* TX processed mbufs back to load_gen */
                        if (rte_mempool_get(mbuf_pool, &meta_data) < 0)
                                rte_panic("Failed to get message buffer\n");
                        *(uint16_t*)(meta_data) = nb_mbuf;
                        rte_ring_enqueue(v2l_que, meta_data);
                        rte_ring_enqueue_bulk(
                            v2l_que, (void**)rt_buf, nb_mbuf, NULL);
                }
        }
        close(socket);
        return 0;
}

/**
 * Main loop of the master core
 * Load image into mbufs -> Enqueue mbufs into l2v_que ring -> Receive processed
 * mbufs from v2l_que -> Calculate latencies and dump results in CSV file
 */
static int proc_loop_master(__attribute__((unused)) void* dummy)
{
        struct rte_mbuf* rt_buf[RT_BUF_SIZE];
        struct mvec* tx_vec = NULL;
        struct mvec* rx_vec = NULL;
        void* meta_data = NULL;
        uint64_t proc_tscs[RT_BUF_SIZE];

        uint16_t i = 0;
        uint16_t r = 0;
        uint16_t img_idx = 0;
        uint16_t nb_mbuf = 0;
        uint16_t tail_size = 0;
        uint64_t last_tsc = 0;
        int ret;

        uint64_t total_tsc_list[IMAGE_FRAME_NUM] = { 0 };
        uint64_t proc_tsc_list[IMAGE_FRAME_NUM] = { 0 };
        double total_delay_ms_list[IMAGE_FRAME_NUM] = { 0.0 };
        double proc_delay_ms_list[IMAGE_FRAME_NUM] = { 0.0 };

        for (r = 0; r < TEST_ROUND; r++) {
                printf("[MASTER] Start test round %u\n", r);
                for (img_idx = 0; img_idx < IMAGE_FRAME_NUM; img_idx++) {
                        last_tsc = rte_get_tsc_cycles();
                        nb_mbuf = gen_rx_buf_from_file("./pikachu.jpg", rt_buf,
                            RT_BUF_SIZE, mbuf_pool, 1500, &tail_size);
                        RTE_LOG(INFO, TEST,
                            "Delay for generate mbufs from file:%.8f ms\n",
                            get_delay_tsc_ms(rte_get_tsc_cycles() - last_tsc));
                        RTE_LOG(INFO, TEST,
                            "[SLAVE] %d mbufs are allocated. Tail size:%d.\n",
                            nb_mbuf, tail_size);

                        /* Enqueue all mbufs in rt_buf into l2v_que ring */
                        tx_vec = mvec_new(rt_buf, nb_mbuf);
                        if (rte_mempool_get(mbuf_pool, &meta_data) < 0)
                                rte_panic("Failed to get message buffer\n");
                        *(uint16_t*)(meta_data) = nb_mbuf;
                        last_tsc = rte_get_tsc_cycles();
                        rte_ring_enqueue(l2v_que, meta_data);
                        rte_ring_enqueue_bulk(
                            l2v_que, (void**)(tx_vec->head), nb_mbuf, NULL);

                        /* Wait until slave return processed mbufs in v2l_que
                         * ring */
                        while (rte_ring_dequeue(v2l_que, &meta_data) < 0) {
                                rte_delay_us_block(DEQUEUE_SLEEP);
                        }
                        nb_mbuf = *((uint16_t*)meta_data);
                        RTE_LOG(INFO, FASTIO_USER,
                            "[SLAVE] %d mbufs are in the v2l_que.\n", nb_mbuf);
                        rte_mempool_put(mbuf_pool, meta_data);
                        rte_ring_dequeue_bulk(
                            v2l_que, (void**)rt_buf, nb_mbuf, NULL);
                        rx_vec = mvec_new(rt_buf, nb_mbuf);

                        mvec_pull_u64(rx_vec, proc_tscs);

                        /* Total time including enqueue/dequeue */
                        total_tsc_list[img_idx]
                            = rte_get_tsc_cycles() - last_tsc;
                        total_delay_ms_list[img_idx]
                            = get_delay_tsc_ms(total_tsc_list[img_idx]);

                        proc_tsc_list[img_idx] = proc_tscs[img_idx];
                        proc_delay_ms_list[img_idx]
                            = get_delay_tsc_ms(proc_tsc_list[img_idx]);

                        RTE_LOG(INFO, FASTIO_USER,
                            "Image index:%u, the total latency is %lu cycles = "
                            "%.8f ms, the processing latency is %lu cycles = "
                            "%0.8f ms.\n",
                            img_idx, total_tsc_list[img_idx],
                            total_delay_ms_list[img_idx],
                            proc_tsc_list[img_idx],
                            proc_delay_ms_list[img_idx]);

                        mvec_free(tx_vec);
                }
                save_double_list_csv(
                    "./proc_delay_ms.csv", proc_delay_ms_list, IMAGE_FRAME_NUM);
                save_double_list_csv("./total_delay_ms.csv", proc_delay_ms_list,
                    IMAGE_FRAME_NUM);
                save_u64_list_csv(
                    "./proc_tsc.csv", proc_tsc_list, IMAGE_FRAME_NUM);
                save_u64_list_csv(
                    "./total_tsc.csv", total_tsc_list, IMAGE_FRAME_NUM);
        }

        return 0;
}

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

        l2v_que = rte_ring_create("L2V_QUE", L2V_QUE_SIZE, rte_socket_id(),
            RING_F_SP_ENQ | RING_F_SC_DEQ);
        v2l_que = rte_ring_create("V2L_QUE", V2L_QUE_SIZE, rte_socket_id(),
            RING_F_SP_ENQ | RING_F_SC_DEQ);
        mbuf_pool = dpdk_init_mempool("test_ring_pool", NUM_MBUFS * nb_ports,
            rte_socket_id(), RTE_MBUF_DEFAULT_BUF_SIZE);

        // Init the device with port_id 0, 1 rx/tx queue
        struct dpdk_device_config cfg = { 0, &mbuf_pool, 1, 1, 256, 256, 1, 1 };
        dpdk_init_device(&cfg);

        if (l2v_que == NULL || v2l_que == NULL || mbuf_pool == NULL) {
                rte_exit(EXIT_FAILURE, "Problem getting rings or mempool.\n");
        }
        print_lcore_infos();

        rte_log_set_global_level(RTE_LOG_EMERG);
        // rte_log_set_global_level(RTE_LOG_INFO);

        RTE_LOG(INFO, TEST, "Launch task on slave cores.\n");
        RTE_LCORE_FOREACH_SLAVE(lcore_id)
        {
                rte_eal_remote_launch(proc_loop_slave, NULL, lcore_id);
                break;
        }

        dpdk_enter_mainloop_master(proc_loop_master, NULL);
        rte_eal_cleanup();
        return 0;
}
