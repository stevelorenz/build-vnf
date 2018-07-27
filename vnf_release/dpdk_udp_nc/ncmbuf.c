/*
 * ncmbuf.c
 *
 * Description: Network code DPDK mbuf with NCKernel
 */

#include "ncmbuf.h"

void check_mbuf_size(struct rte_mempool* mbuf_pool, struct nck_encoder* enc,
    uint16_t header_size)
{
        RTE_LOG(DEBUG, USER1, "Mbuf data room: %u B\n",
            rte_pktmbuf_data_room_size(mbuf_pool));
        if ((rte_pktmbuf_data_room_size(mbuf_pool) - header_size)
            < (uint16_t)(enc->coded_size)) {
                rte_exit(EXIT_FAILURE,
                    "The size of mbufs is not enough for coding.\n");
        }
}

uint8_t encode_udp(
    struct rte_mbuf* m_in, struct rte_mempool* mbuf_pool, uint16_t port_id)
{
        struct rte_mbuf* m_out;
        m_out = rte_pktmbuf_clone(m_in, mbuf_pool);
        rte_pktmbuf_free(m_in);
        rte_pktmbuf_free(m_out);
        printf("%d", port_id);
        return 0;
}
