/*
 * About:
 */

#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <rte_cycles.h>
#include <rte_eal.h>

#include <fastio_user/device.h>
#include <fastio_user/io.h>
#include <fastio_user/ipc.h>
#include <fastio_user/memory.h>
#include <fastio_user/task.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NUM_MBUFS 5000
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define RX_BUF_SIZE 500
#define TEST_IMG_NUM 100

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

uint64_t get_unix_ts_micro()
{
        struct timespec tms;
        /* POSIX.1-2008 way */
        if (clock_gettime(CLOCK_REALTIME, &tms)) {
                return -1;
        }
        /* seconds, multiplied with 1 million */
        int64_t micros = tms.tv_sec * 1000000;
        /* Add full microseconds */
        micros += tms.tv_nsec / 1000;
        /* round up if necessary */
        if (tms.tv_nsec % 1000 >= 500) {
                ++micros;
        }
        return micros;
}

static int proc_loop(__attribute__((unused)) void* dummy)
{
        struct rte_mbuf* rx_buf[RX_BUF_SIZE];
        uint16_t nb_mbuf = 0;
        uint16_t tail_size = 0;
        int socket = 0;
        uint16_t i = 0;
        char img_path[50];
        uint64_t rx_buffer_send_ts_arr[TEST_IMG_NUM];
        FILE* fd;

        socket = init_uds_stream_cli("/uds_socket");
        for (i = 0; i < TEST_IMG_NUM; ++i) {
                sprintf(
                    img_path, "/app/yolov2/cocoapi/images/val2017/%d.jpg", i);
                nb_mbuf = gen_rx_buf_from_file(
                    img_path, rx_buf, RX_BUF_SIZE, mbuf_pool, 1500, &tail_size);
                /*printf("%d mbufs are allocated. Tail size:%d.\n", nb_mbuf,*/
                /*tail_size);*/

                rx_buffer_send_ts_arr[i] = get_unix_ts_micro();
                send_mbufs_data_uds(socket, rx_buf,
                    1500 * (nb_mbuf - 1) + tail_size, nb_mbuf, tail_size);
                sleep(0.5);
                dpdk_free_buf(rx_buf, nb_mbuf);
        }

        // Store array of timestamps
        fd = fopen("./rx_buffer_send_ts.csv", "a+");
        for (i = 0; i < TEST_IMG_NUM; ++i) {
                fprintf(fd, "%lu\n", rx_buffer_send_ts_arr[i]);
        }
        fclose(fd);
        return 0;
}

/**
 * @brief main
 *
 */
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

        mbuf_pool = dpdk_init_mempool("l2_fwd_pool", NUM_MBUFS * nb_ports,
            rte_socket_id(), RTE_MBUF_DEFAULT_BUF_SIZE);

        // Init the device with port_id 0
        struct dpdk_device_config cfg = { 0, &mbuf_pool, 1, 1, 256, 256, 1, 1 };
        dpdk_init_device(&cfg);

        dpdk_enter_mainloop_master(proc_loop);

        return 0;
}
