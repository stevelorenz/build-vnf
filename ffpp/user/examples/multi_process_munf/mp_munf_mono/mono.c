/*
 * mono.c
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rte_eal.h>

#include <rte_cycles.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ring.h>

#include <ffpp/aes.h>
#include <ffpp/collections.h>
#include <ffpp/config.h>
#include <ffpp/general_helpers_user.h>
#include <ffpp/munf.h>
#include <ffpp/packet_processors.h>

#define BURST_SIZE 64

static volatile bool force_quit = false;

static struct rte_ether_addr tx_port_addr;

static int func_num = 0;

static uint8_t aes_key[] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
			     0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
			     0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
			     0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4 };
static uint8_t aes_iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		force_quit = true;
	}
}

static void run_update_dl_dst(struct ffpp_mvec *vec)
{
	ffpp_pp_update_dl_dst(vec, &tx_port_addr);
}

static void run_l2_xor(struct ffpp_mvec *vec)
{
	uint16_t i, j;
	struct rte_mbuf *m;
	struct rte_ether_hdr *eth;
	const uint8_t xor_val = 17;
	uint8_t *data;

	FFPP_MVEC_FOREACH(vec, i, m)
	{
		rte_prefetch0(rte_pktmbuf_mtod(m, void *));
		data = rte_pktmbuf_mtod(m, uint8_t *);
		for (j = 0; j < 1500; ++j) {
			*(data + j) ^= xor_val;
		}
	}
}

static void run_l2_aes(struct ffpp_mvec *vec)
{
	uint16_t i, j;
	struct rte_mbuf *m;
	struct rte_ether_hdr *eth;
	const uint8_t xor_val = 17;
	uint8_t *data;
	struct AES_ctx aes_ctx;

	FFPP_MVEC_FOREACH(vec, i, m)
	{
		rte_prefetch0(rte_pktmbuf_mtod(m, void *));
		data = rte_pktmbuf_mtod(m, uint8_t *);
		AES_init_ctx_iv(&aes_ctx, aes_key, aes_iv);
		AES_CBC_encrypt_buffer(&aes_ctx, data, 1500);
		AES_init_ctx_iv(&aes_ctx, aes_key, aes_iv);
		AES_CBC_decrypt_buffer(&aes_ctx, data, 1500);
	}
}

void run_mainloop(const struct ffpp_munf_manager *ctx)
{
	struct rte_mbuf *pkt_burst[BURST_SIZE];
	uint16_t nb_rx, nb_tx;

	struct ffpp_mvec vec;
	ffpp_mvec_init(&vec, BURST_SIZE);
	uint16_t i;

	while (!force_quit) {
		nb_rx = rte_eth_rx_burst(ctx->rx_port_id, 0, pkt_burst,
					 BURST_SIZE);
		if (nb_rx == 0) {
			continue;
		}
		ffpp_mvec_set_mbufs(&vec, pkt_burst, nb_rx);

		switch (func_num) {
		case 0:
			run_update_dl_dst(&vec);
			break;
		case 1:
			run_l2_xor(&vec);
			run_l2_xor(&vec);
			run_update_dl_dst(&vec);
			break;
		case 2:
			run_l2_aes(&vec);
			run_update_dl_dst(&vec);
			break;
		default:
			rte_exit(EXIT_FAILURE, "Unknown function type!\n");
		}

		// No buffering is used like l2fwd.
		rte_eth_tx_burst(ctx->tx_port_id, 0, pkt_burst, nb_rx);
	}
	ffpp_mvec_free(&vec);
}

static void parse_args(int argc, char *argv[])
{
	int opt = 0;

	while ((opt = getopt(argc, argv, "f:")) != -1) {
		switch (opt) {
		case 'f':
			func_num = atoi(optarg);
			break;
		default:
			rte_exit(EXIT_FAILURE, "Can not parse arguments!\n");
		}
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	argc -= ret;
	argv += ret;
	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	parse_args(argc, argv);
	printf("The function number: %d\n", func_num);

	struct ffpp_munf_manager munf_manager;
	struct rte_mempool *pool = NULL;
	ffpp_munf_init_manager(&munf_manager, "test_manager", pool);
	ret = rte_eth_macaddr_get(munf_manager.tx_port_id, &tx_port_addr);
	if (ret < 0) {
		rte_exit(EXIT_FAILURE, "Cannot get the MAC address.\n");
	}

	run_mainloop(&munf_manager);

	ffpp_munf_cleanup_manager(&munf_manager);
	rte_eal_cleanup();
	return 0;
}
