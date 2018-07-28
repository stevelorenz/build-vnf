/*
 * ncmbuf.c
 *
 * Description: Network code DPDK mbuf with NCKernel
 */

#include "ncmbuf.h"

void check_mbuf_size(struct rte_mempool* mbuf_pool, struct nck_encoder* enc)
{
        RTE_LOG(DEBUG, USER1, "Mbuf pool data room size: %u bytes\n",
            rte_pktmbuf_data_room_size(mbuf_pool));
        if ((rte_pktmbuf_data_room_size(mbuf_pool) - UDP_NC_DATA_HEADER_LEN)
            < (uint16_t)(enc->coded_size)) {
                rte_exit(EXIT_FAILURE,
                    "The size of mbufs is not enough for coding.\n");
        }
}

uint8_t encode_udp(
    struct rte_mbuf* m_in, struct rte_mempool* mbuf_pool, uint16_t portid)
{
        struct rte_mbuf* m_out;
        m_out = rte_pktmbuf_clone(m_in, mbuf_pool);

#if defined(DEBUG) && (DEBUG == 0)
        recode_udp(m_out, mbuf_pool, portid);
        return 0;
#endif

        l2fwd_put_rxq(m_out, portid);

        rte_pktmbuf_free(m_in);
        printf("%d", portid);
        return 0;
}

uint8_t recode_udp(
    struct rte_mbuf* m_in, struct rte_mempool* mbuf_pool, uint16_t portid)
{
        struct rte_mbuf* m_out;
        m_out = rte_pktmbuf_clone(m_in, mbuf_pool);
        l2fwd_put_rxq(m_out, portid);
        rte_pktmbuf_free(m_in);
        printf("%d", portid);
        return 0;
}
