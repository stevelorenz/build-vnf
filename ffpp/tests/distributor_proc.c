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

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
// The queue configuration of each lcore
// Assumptions:
// - One port has only one RX/TX queue.
struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
	struct lcore_rx_queue rx_queue_list[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

#define MAX_PKT_BURST 32

static uint64_t timer_period = 0;

static void quit_signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
		       signum);
		force_quit = true;
	}
}

static void l2fwd_simple_forward(struct rte_mbuf *m, unsigned portid)
{
    struct rte_ether_hdr *eth;
    void *tmp;
    unsigned dst_port;

    dst_port = l2fwd_dst_ports[portid];

    eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);

    /* 02:00:00:00:00:xx */

    tmp = &eth->d_addr.addr_bytes[0];

    *((uint64_t *)tmp) = 0x000000000002 + ((uint64_t) dst_port << 40);

    /* src addr */

    rte_ether_addr_copy(&l2fwd_ports_eth_addr[dst_port], &eth->s_addr);

    l2fwd_send_packet(m, (uint8_t) dst_port);
}


static int proc_loop_master(__attribute__((unused)) void *dummy)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	int sent;
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
	unsigned i, j, portid, nb_rx;
	while (!force_quit) 
	{
		//rte_delay_ms(500);
		cur_tsc = rte_rdtsc();

		/*
 		*   TX burst queue drain
 		*/

		diff_tsc = cur_tsc - prev_tsc;

		if (unlikely(diff_tsc > drain_tsc)) 
		{
    		    for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) 
		    {
        		if (qconf->tx_mbufs[portid].len == 0)
            		    continue;
	
        		l2fwd_send_burst(&lcore_queue_conf[lcore_id], 
					qconf->tx_mbufs[portid].len, 
					(uint8_t) portid);

        		qconf->tx_mbufs[portid].len = 0;
    		    }
	
    		    /* if timer is enabled */
    		    if (timer_period > 0)
		    {
        	        /* advance the timer */
        	        timer_tsc += diff_tsc;

        	        /* if timer has reached its timeout */
        	        if (unlikely(timer_tsc >= (uint64_t) timer_period)) 
			{

	                    /* do this only on master core */

	                    if (lcore_id == rte_get_master_lcore()) 
			    {
	                	print_stats();

	                	/* reset the timer */
	                	timer_tsc = 0;
            		    }
        	    	}
    		    }
		    prev_tsc = cur_tsc;
		}
		
		/*
 		* Read packet from RX queues
 		*/

		for (i = 0; i < qconf->n_rx_port; i++) 
		{
    		    portid = qconf->rx_port_list[i];
    		    nb_rx = rte_eth_rx_burst((uint8_t) portid, 0,  pkts_burst,
					    MAX_PKT_BURST);

		    for (j = 0; j < nb_rx; j++) 
		    {
        		m = pkts_burst[j];
        		rte_prefetch0[rte_pktmbuf_mtod(m, void *)); 
			l2fwd_simple_forward(m, portid);
    		    }
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

	// create the mbuf pool
	l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF,
  	MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
   	rte_socket_id());

	if (l2fwd_pktmbuf_pool == NULL)
	    rte_panic("Cannot init mbuf pool\n");
	
	if (rte_pci_probe() < 0)
	    rte_exit(EXIT_FAILURE, "Cannot probe PCI\n");

	// reset l2fwd_dst_ports
	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
    	     l2fwd_dst_ports[portid] = 0;

	last_port = 0;

	/*
 	* Each logical core is assigned a dedicated TX queue on each port.
 	*/

	RTE_ETH_FOREACH_DEV(portid)
	{
	    struct rte_eth_rxconf rxq_conf;
	    struct rte_eth_txconf txq_conf;

    	// skip ports that are not enabled
    	    if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
       	        continue;

    	    if (nb_ports_in_mask % 2) {
                l2fwd_dst_ports[portid] = last_port;
                l2fwd_dst_ports[last_port] = portid;
    	    }
    	    else
       	        last_port = portid;

    	    nb_ports_in_mask++;

    	    rte_eth_dev_info_get((uint8_t) portid, &dev_info);
	    ret = rte_eth_dev_configure((uint8_t)portid, 1, 1, &port_conf);
	    if (ret < 0)
  	        rte_exit(EXIT_FAILURE, "Cannot configure device: "
        	    "err=%d, port=%u\n",
        	    ret, portid);
	    // Init RX queue
	    ret = rte_eth_rx_queue_setup((uint8_t) portid, 0, nb_rxd, 
					SOCKET0, &rx_conf,
					l2fwd_pktmbuf_pool);
	    if (ret < 0)
  	        rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup: "
        	    "err=%d, port=%u\n",
        	    ret, portid);
	    // Init one TX queue on each port

	    fflush(stdout);

	    ret = rte_eth_tx_queue_setup((uint8_t) portid, 0, nb_txd, 
					rte_eth_dev_socket_id(portid), 
					&tx_conf);
	    if (ret < 0)
    		rte_exit(EXIT_FAILURE, 
			"rte_eth_tx_queue_setup:err=%d, port=%u\n", 
			ret, (unsigned) portid);
	}


	
	signal(SIGINT, quit_signal_handler);
	signal(SIGTERM, quit_signal_handler);

	// MARK: Hard-coded for testing
	const unsigned nb_ports = 1;
	printf("%u ports were found\n", nb_ports);
	if (nb_ports != 1)
		rte_exit(EXIT_FAILURE, "Error: ONLY support one port! \n");

	mbuf_pool =
		dpdk_init_mempool("distributor_pool", NUM_MBUFS * nb_ports,
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
