/*
 * packet_processors.c
 */

#include <rte_mbuf.h>

#include <ffpp/packet_processors.h>

void ffpp_pp_update_dl_dst(struct ffpp_mvec *vec,
			   const struct rte_ether_addr *dl_dst)
{
	uint16_t i;
	struct rte_mbuf *m;
	struct rte_ether_hdr *eth;

	FFPP_MVEC_FOREACH(vec, i, m)
	{
		rte_prefetch0(rte_pktmbuf_mtod(m, void *));
		eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
		rte_ether_addr_copy(dl_dst, &eth->s_addr);
	}
}
