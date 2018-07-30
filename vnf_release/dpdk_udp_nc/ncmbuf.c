/*
 * ncmbuf.c
 *
 * Description: Network code DPDK mbuf with NCKernel
 */

#include <rte_branch_prediction.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_launch.h>
#include <rte_lcore.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_memory.h>
#include <rte_mempool.h>
#include <rte_per_lcore.h>
#include <rte_random.h>
#include <rte_ring.h>
#include <rte_udp.h>

#include "ncmbuf.h"

#define MAGIC_NUM 117
#define NC_DATA_HDR_LEN 1
#define UDP_HDR_LEN 8

static inline __attribute__((always_inline)) void recalc_cksum(
    struct ipv4_hdr* iph, struct udp_hdr* udph)
{
        udph->dgram_cksum = 0;
        iph->hdr_checksum = 0;
        udph->dgram_cksum = rte_ipv4_udptcp_cksum(iph, udph);
        iph->hdr_checksum = rte_ipv4_cksum(iph);
}

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

uint8_t encode_udp(struct nck_encoder* enc, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t))
{
        static struct sk_buff skb;
        struct rte_mbuf* m_out = NULL;
        uint16_t in_data_len;
        uint16_t num_coded = 0;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint8_t* pt_data;

        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph
            + ((iph->version_ihl & 0x0F) * 32 / 8));
        in_data_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;
        pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;

        /* Use m_in as the encoding buffer */
        rte_pktmbuf_append(
            m_in, (enc->coded_size - in_data_len) + NC_DATA_HDR_LEN);

        skb_new(&skb, pt_data, enc->coded_size + NC_DATA_HDR_LEN);
        skb_reserve(&skb, sizeof(uint16_t));
        skb_put(&skb, in_data_len);
        /* Push data length since the data can be padded by the encoder */
        skb_push_u16(&skb, skb.len);
        nck_put_source(enc, &skb);

        while (nck_has_coded(enc)) {
                num_coded += 1;
                m_out = rte_pktmbuf_clone(m_in, mbuf_pool);
                iph = rte_pktmbuf_mtod_offset(
                    m_out, struct ipv4_hdr*, ETHER_HDR_LEN);
                udph = (struct udp_hdr*)((char*)iph
                    + ((iph->version_ihl & 0x0F) * 32 / 8));
                pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;
                skb_new(&skb, pt_data, enc->coded_size + NC_DATA_HDR_LEN);
                skb_reserve(&skb, sizeof(uint8_t));
                nck_get_coded(enc, &skb);
                skb_push_u8(&skb, MAGIC_NUM);
                RTE_LOG(DEBUG, USER1,
                    "[ENC] Output num: %u, input len: %u, output len: "
                    "%u.\n",
                    num_coded, in_data_len, skb.len);
                udph->dgram_len = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                iph->total_length
                    = rte_cpu_to_be_16(rte_be_to_cpu_16(iph->total_length)
                        + (skb.len - in_data_len));
                recalc_cksum(iph, udph);
                (*put_rxq)(m_out, portid);
        }

        rte_pktmbuf_free(m_in);

        return 0;
}

uint8_t recode_udp(struct nck_recoder* rec, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t))
{

        static struct sk_buff skb;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint16_t in_data_len;
        uint16_t num_recoded = 0;
        struct rte_mbuf* m_out = NULL;
        uint8_t* pt_data;

        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph
            + ((iph->version_ihl & 0x0F) * 32 / 8));
        in_data_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;
        pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;

        skb_new(&skb, pt_data, in_data_len);
        skb_put(&skb, in_data_len);
        if (skb_pull_u8(&skb) != MAGIC_NUM) {
                rte_pktmbuf_free(m_in);
                RTE_LOG(DEBUG, USER1, "[REC] Recv an invalid input.\n");
                return 0;
        }
        nck_put_coded(rec, &skb);

        if (!nck_has_coded(rec)) {
                RTE_LOG(DEBUG, USER1, "[REC] Recoder has no coded output.\n");
                (*put_rxq)(m_in, portid); // forward received packets
                return 0;
        }

        while (nck_has_coded(rec)) {
                /* Clone a new mbuf for output */
                num_recoded += 1;
                m_out = rte_pktmbuf_clone(m_in, mbuf_pool);
                iph = rte_pktmbuf_mtod_offset(
                    m_out, struct ipv4_hdr*, ETHER_HDR_LEN);
                udph = (struct udp_hdr*)((char*)iph
                    + ((iph->version_ihl & 0x0F) * 32 / 8));
                pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;

                skb_new(&skb, pt_data, in_data_len);
                skb_reserve(&skb, sizeof(uint8_t));
                nck_get_coded(rec, &skb);
                skb_push_u8(&skb, MAGIC_NUM);
                RTE_LOG(DEBUG, USER1,
                    "[REC] Output num: %u, input len: %u, output len: %u.\n",
                    num_recoded, in_data_len, skb.len);
                rte_pktmbuf_append(m_out, (skb.len - in_data_len));

                udph->dgram_len = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                iph->total_length
                    = rte_cpu_to_be_16(rte_be_to_cpu_16(iph->total_length)
                        + (skb.len - in_data_len));
                recalc_cksum(iph, udph);
                (*put_rxq)(m_out, portid);
        }
        rte_pktmbuf_free(m_in);
        return 0;
}

uint8_t decode_udp(struct nck_decoder* dec, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t))
{

        static struct sk_buff skb;
        struct rte_mbuf* m_out = NULL;
        uint16_t in_data_len;
        uint16_t num_decoded = 0;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint8_t* pt_data;

        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph
            + ((iph->version_ihl & 0x0F) * 32 / 8));
        in_data_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;
        pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;

        /* Use m_in as the decoding buffer */
        skb_new(&skb, pt_data, in_data_len);
        skb_put(&skb, in_data_len);
        if (skb_pull_u8(&skb) != MAGIC_NUM) {
                rte_pktmbuf_free(m_in);
                RTE_LOG(DEBUG, USER1, "[DEC] Recv an invalid input.\n");
                return 0;
        }
        nck_put_coded(dec, &skb);

        if (!nck_has_source(dec)) {
                rte_pktmbuf_free(m_in);
                RTE_LOG(DEBUG, USER1, "[DEC] Decoder has no output.\n");
                return 0;
        }

        while (nck_has_source(dec)) {
                num_decoded += 1;
                m_out = rte_pktmbuf_clone(m_in, mbuf_pool);
                iph = rte_pktmbuf_mtod_offset(
                    m_out, struct ipv4_hdr*, ETHER_HDR_LEN);
                udph = (struct udp_hdr*)((char*)iph
                    + ((iph->version_ihl & 0x0F) * 32 / 8));
                pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;
                skb_new(&skb, pt_data, in_data_len);
                nck_get_source(dec, &skb);
                skb.len = skb_pull_u16(&skb);
                rte_pktmbuf_trim(m_out, (in_data_len - skb.len));
                RTE_LOG(DEBUG, USER1,
                    "[DEC] Output num: %u, input len: %u,output len: %u.\n",
                    num_decoded, in_data_len, skb.len);
                udph->dgram_len = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                iph->total_length
                    = rte_cpu_to_be_16(rte_be_to_cpu_16(iph->total_length)
                        - (in_data_len - skb.len));
                recalc_cksum(iph, udph);
                (*put_rxq)(m_out, portid);
        }

        rte_pktmbuf_free(m_in);

        return 0;
}
