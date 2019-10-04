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

/* Use rings to emulate NIC queues */
struct rte_ring *in_que, *out_que;

static void quit_signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
		       signum);
		force_quit = true;
	}
}

void check_pull_values(uint64_t *arr, uint16_t len)
{
	uint16_t i;
	for (i = 0; i < len; ++i) {
		if (arr[i] != FFBB_MAGIC) {
			rte_panic("ERROR! Pulled values are wrong!");
		}
	}
}

/**
 * Process or operate on a mbuf vector.
 * This functions does not perform meaningful processing. It is used to test
 * mvec helper functions. The mbuf vector should be the same as the input at
 * the end.
 */
void test_mvec_helpers(struct mvec *v)
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
}

static int proc_loop_slave(__attribute__((unused)) void *dummy)
{
	struct rte_mbuf *rx_buf[RX_BUF_SIZE];
	struct rte_mbuf *read_buf[RX_BUF_SIZE];
	void *meta_data = NULL;
	uint16_t nb_mbuf = 0;
	uint16_t i = 0;
	uint16_t tail_size = 0;
	struct mvec *vec_recv = NULL;
	struct mvec *vec_read = NULL;
	int ret = 0;

	/* Dequeue metadata packet */
	while (rte_ring_dequeue(in_que, &meta_data) < 0) {
		usleep(50);
	}
	nb_mbuf = *((uint16_t *)meta_data);
	RTE_LOG(INFO, TEST, "[MASTER] %d mbufs are in the in_queue.\n",
		nb_mbuf);
	rte_mempool_put(mbuf_pool, meta_data);

	/* Dequeue data packets */
	rte_ring_dequeue_bulk(in_que, (void **)rx_buf, nb_mbuf, NULL);

	/* Test mvec operations, push, pull etc */
	vec_recv = mvec_new(rx_buf, nb_mbuf);

	RTE_LOG(INFO, TEST, "Before mvec processing.\n");
	print_mvec(vec_recv);
	test_mvec_helpers(vec_recv);
	RTE_LOG(INFO, TEST, "After mvec processing.\n");
	print_mvec(vec_recv);

	/* Check if TX/RX and mbuf vector operations work properly */
	nb_mbuf = gen_rx_buf_from_file("./pikachu.jpg", read_buf, RX_BUF_SIZE,
				       mbuf_pool, 1500, &tail_size);
	vec_read = mvec_new(read_buf, nb_mbuf);
	print_mvec(vec_read);

	ret = mvec_datacmp(vec_recv, vec_read);
	if (ret != 0) {
		rte_panic(
			"ERROR! Received data is different from the sent data.\n");
	}

	RTE_LOG(INFO, TEST, "Free the mbuf vectors.\n");
	mvec_free(vec_recv);
	mvec_free(vec_read);
	return 0;
}

static int proc_loop_master(__attribute__((unused)) void *dummy)
{
	struct rte_mbuf *tx_buf[RX_BUF_SIZE];
	uint16_t i;
	uint16_t nb_mbuf = 0;
	uint16_t ring_size = 0;
	uint16_t free_space = 0;
	uint16_t tail_size = 0;
	void *meta_data = NULL;

	nb_mbuf = gen_rx_buf_from_file("./pikachu.jpg", tx_buf, RX_BUF_SIZE,
				       mbuf_pool, 1500, &tail_size);

	RTE_LOG(INFO, TEST, "[SLAVE] %d mbufs are allocated. Tail size:%d.\n",
		nb_mbuf, tail_size);

	/* Enqueue first the number of mbufs, then all mbufs */
	if (rte_mempool_get(mbuf_pool, &meta_data) < 0)
		rte_panic("Failed to get message buffer\n");
	*(uint16_t *)(meta_data) = nb_mbuf;
	rte_ring_enqueue(in_que, meta_data);
	rte_ring_enqueue_bulk(in_que, (void **)tx_buf, nb_mbuf, NULL);

	ring_size = rte_ring_get_size(in_que);
	free_space = rte_ring_free_count(in_que);
	RTE_LOG(INFO, TEST,
		"[SLAVE] Enqueue finished, in_que size:%u, free space of "
		"in_que:%u\n",
		ring_size, free_space);

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

	in_que = rte_ring_create("IN_QUE", IN_QUE_SIZE, rte_socket_id(),
				 RING_F_SP_ENQ | RING_F_SC_DEQ);
	out_que = rte_ring_create("OUT_QUE", OUT_QUE_SIZE, rte_socket_id(),
				  RING_F_SP_ENQ | RING_F_SC_DEQ);
	mbuf_pool =
		dpdk_init_mempool("test_ring_pool", NUM_MBUFS * nb_ports,
				  rte_socket_id(), RTE_MBUF_DEFAULT_BUF_SIZE);

	// Init the device with port_id 0
	struct dpdk_device_config cfg = { 0, &mbuf_pool, 1, 1, 256, 256, 1, 1 };
	dpdk_init_device(&cfg);

	if (in_que == NULL || out_que == NULL || mbuf_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Problem getting rings or mempool\n");
	}

	print_lcore_infos();

	RTE_LOG(INFO, TEST, "Launch task on the first slave core.\n");
	RTE_LCORE_FOREACH_SLAVE(lcore_id)
	{
		rte_eal_remote_launch(proc_loop_slave, NULL, lcore_id);
		break;
	}

	dpdk_enter_mainloop_master(proc_loop_master, NULL);

	rte_eal_cleanup();
	return 0;
}
