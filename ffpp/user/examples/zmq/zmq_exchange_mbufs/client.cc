/*
 * client.cc
 */

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#include <ffpp/config.h>
#include <ffpp/munf.h>

#include <zmq.h>

static volatile bool force_quit = false;
static const uint16_t BURST_SIZE = 64;
static struct rte_ether_addr tx_port_addr;

static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		force_quit = true;
	}
}

void run_mainloop(const struct ffpp_munf_manager *manager)
{
	struct rte_mbuf *pkt_burst[BURST_SIZE];
	uint16_t nb_rx, nb_tx;
	uint16_t i;
	uint64_t prev_tsc = 0, diff_tsc;
	float dur = 0.0;
	void *context = zmq_ctx_new();
	uint8_t *data;
	uint32_t zmq_recv_nbytes = 0;

	void *req = zmq_socket(context, ZMQ_REQ);
	zmq_connect(req, "ipc:///tmp/ffpp.sock");

	printf("Enter RX/TX loop...\n");
	while (!force_quit) {
		nb_rx = rte_eth_rx_burst(manager->rx_port_id, 0, pkt_burst,
					 BURST_SIZE);
		if (nb_rx == 0) {
			continue;
		}
		prev_tsc = rte_rdtsc();

		for (i = 0; i < nb_rx; ++i) {
			data = rte_pktmbuf_mtod(pkt_burst[i], uint8_t *);
			zmq_send(req, data, 64, 0);
			for (;;) {
				// The data processing in slow path MUST reduce the payload.
				zmq_recv_nbytes =
					zmq_recv(req, data, 64, ZMQ_DONTWAIT);

				if (zmq_recv_nbytes != -1) {
					assert(zmq_recv_nbytes == 32);
					break;
				} else if (force_quit == true) {
					goto end_main_loop;
				}

				rte_delay_us_block(1e3);
			}
		}

		diff_tsc = rte_rdtsc() - prev_tsc;
		dur = ((float)(diff_tsc) / rte_get_timer_hz()) * 1000;
		printf("The cost of one REQ-REP. Cycles: %lu, time: %fms\n",
		       diff_tsc, dur);
		// No buffering is used like l2fwd.
		rte_eth_tx_burst(manager->tx_port_id, 0, pkt_burst, nb_rx);
	}
end_main_loop:
	zmq_close(req);
	zmq_ctx_destroy(context);
}

int main(int argc, char *argv[])
{
	int ret;
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments.\n");
	}
	argc -= ret;
	argv += ret;
	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	struct ffpp_munf_manager munf_manager;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_manager(&munf_manager, "test_manager", pool);
	ret = rte_eth_macaddr_get(munf_manager.tx_port_id, &tx_port_addr);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Cannot get the MAC address.\n");
	}

	run_mainloop(&munf_manager);

	printf("Main loop ends, run cleanups...\n");
	ffpp_munf_cleanup_manager(&munf_manager);
	rte_eal_cleanup();
	return 0;
}
