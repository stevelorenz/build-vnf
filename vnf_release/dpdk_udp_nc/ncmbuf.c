/*
 * ncmbuf.c
 *
 */

#include <rte_branch_prediction.h>
#include <rte_common.h>
#include <rte_cycles.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_log.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_mempool.h>
#include <rte_udp.h>

#include <dpdk_helper.h>

#include "ncmbuf.h"

#define NC_HDR_LEN 1
#define UDP_HDR_LEN 8

#define MAGIC_NUM 117

#define RTE_LOGTYPE_NCMBUF RTE_LOGTYPE_USER1

static inline __attribute__((always_inline)) void recalc_cksum(
    struct ipv4_hdr* iph, struct udp_hdr* udph)
{
        udph->dgram_cksum = 0;
        iph->hdr_checksum = 0;
        udph->dgram_cksum = rte_ipv4_udptcp_cksum(iph, udph);
        iph->hdr_checksum = rte_ipv4_cksum(iph);
}

void check_mbuf_size(
    struct rte_mempool* mbuf_pool, struct nck_encoder* enc, uint16_t hdr_len)
{
        if ((rte_pktmbuf_data_room_size(mbuf_pool) - hdr_len - NC_HDR_LEN)
            < (uint16_t)(enc->coded_size)) {
                rte_exit(EXIT_FAILURE,
                    "The size of mbufs is not enough for coding. mbuf data"
                    "room:%u, reserved header size:%u, encoder's coded "
                    "size:%lu, NC header len:%u\n",
                    rte_pktmbuf_data_room_size(mbuf_pool), hdr_len,
                    enc->coded_size, NC_HDR_LEN);
        }
}

/* TODO:  <01-08-18, Zuo> Split this funciton into encode_input and
 * encode_output. These functions can be used to implement recoder
 * */

uint8_t encode_udp_data(struct nck_encoder* enc, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t))
{
        static struct sk_buff skb;
        struct rte_mbuf* m_base = NULL;
        struct rte_mbuf* m_out = NULL;
        uint16_t in_data_len;
        uint16_t in_iphdr_len;
        uint16_t num_coded = 0;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint8_t* pt_data;
        int ret;

        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        in_iphdr_len = (iph->version_ihl & 0x0F) * 32 / 8;
        udph = (struct udp_hdr*)((char*)iph + in_iphdr_len);
        in_data_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;
        /*pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;*/

        /* Make a pre-deepcopy of the input mbuf (m_in).
         * Avoid m_in could be freed after being put into the TX queue.
         * m_base is used to create deep copies for redundant coded UDP
         * segments.
         * */
        m_base = mbuf_udp_deep_copy(
            m_in, mbuf_pool, (ETHER_HDR_LEN + in_iphdr_len + UDP_HDR_LEN));

        /* IDEA: The original mbuf is used to be copied to the encoder's buffer
         * Because the encoder could pad zeros if the input length is smaller
         * than coded_size, the length of original data could be lost. So the
         * original length should also be coded.
         * 1. Prepend data_len at the beginning -> either whole header part of
         * data part have to be moved to reserve 2 bytes length.
         * 2. Append data_len at the end. No movements are required. But the
         * recoder cannot find the original end position.
         *
         * So method 1 SHOULD be used. header part(usually smaller) is moved.
         * */
        pt_data = (uint8_t*)(rte_pktmbuf_prepend(m_in, sizeof(uint16_t)));
        rte_memcpy(pt_data, pt_data + sizeof(uint16_t),
            (ETHER_HDR_LEN + in_iphdr_len + UDP_HDR_LEN));
        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        udph = (struct udp_hdr*)((char*)iph + in_iphdr_len);
        pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;
        in_data_len = rte_cpu_to_be_16(in_data_len);
        rte_memcpy(pt_data, &in_data_len, sizeof(uint16_t));
        in_data_len = rte_be_to_cpu_16(in_data_len);
        RTE_VERIFY(rte_cpu_to_be_16(in_data_len) == *(uint16_t*)(pt_data));

        skb_new(&skb, pt_data, enc->coded_size + NC_HDR_LEN);
        skb_put(&skb, in_data_len + sizeof(uint16_t));
        /* MARK: This step copy skb->data(mbuf's data memory) to encoder's own
         * buffer */
        nck_put_source(enc, &skb);

        /* MARK: by rte_pktmbuf_clone "cloned" mbufs share the same data domain.
         *       refcnt ++, the m_clone->pkt.data = m_in->pkt.data for encoder
         *       output(s), new mbufs should be fully created(deep copy)
         *       instead.
         */
        while (nck_has_coded(enc)) {
                num_coded += 1;
                /*
                 * num_coded == 1: Use m_in for output, no deep copy is required
                 * num_coded >= 2: Make a deep copy of m_base.
                 * */
                if (num_coded == 1) {
                        skb_new(&skb, pt_data, enc->coded_size + NC_HDR_LEN);
                        skb_reserve(&skb, NC_HDR_LEN);
                        /* MARK: SHOULD look into it for memcpy, rte_memcpy
                         * support vectorized copy */
                        nck_get_coded(enc, &skb);
                        skb_push_u8(&skb, MAGIC_NUM);
                        rte_pktmbuf_append(m_in, (skb.len - in_data_len));
                        udph->dgram_len
                            = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                        iph->total_length = rte_cpu_to_be_16(
                            skb.len + UDP_HDR_LEN + in_iphdr_len);
                        recalc_cksum(iph, udph);
                        (*put_rxq)(m_in, portid);
                } else {
                        m_out = mbuf_udp_deep_copy(m_base, mbuf_pool,
                            (ETHER_HDR_LEN + in_iphdr_len + UDP_HDR_LEN));
                        iph = rte_pktmbuf_mtod_offset(
                            m_out, struct ipv4_hdr*, ETHER_HDR_LEN);
                        udph = (struct udp_hdr*)((char*)iph
                            + ((iph->version_ihl & 0x0F) * 32 / 8));
                        pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;
                        skb_new(&skb, pt_data, enc->coded_size + NC_HDR_LEN);
                        skb_reserve(&skb, NC_HDR_LEN);
                        ret = nck_get_coded(enc, &skb);
                        skb_push_u8(&skb, MAGIC_NUM);
                        RTE_LOG(DEBUG, NCMBUF,
                            "[ENC] Redundant seg: %u, ret: %d, output len: "
                            "%u\n",
                            (num_coded - 1), ret, skb.len);
                        rte_pktmbuf_append(m_out, skb.len);
                        udph->dgram_len
                            = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                        iph->total_length = rte_cpu_to_be_16(
                            skb.len + UDP_HDR_LEN + in_iphdr_len);
                        recalc_cksum(iph, udph);
                        (*put_rxq)(m_out, portid);
                }

                RTE_LOG(DEBUG, NCMBUF,
                    "[ENC] Output num: %u, input len: %u, coded_size: "
                    "%lu, output len: %u. ip_len:%u, udp_len:%u\n",
                    num_coded, in_data_len, enc->coded_size, skb.len,
                    rte_be_to_cpu_16(iph->total_length),
                    rte_be_to_cpu_16(udph->dgram_len));
        }

        rte_pktmbuf_free(m_base);

        return 0;
}

uint8_t recode_udp_data(struct nck_recoder* rec, struct rte_mbuf* m_in,
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
                RTE_LOG(DEBUG, NCMBUF, "[REC] Recv an invalid input.\n");
                return 0;
        }
        nck_put_coded(rec, &skb);

        if (!nck_has_coded(rec)) {
                RTE_LOG(DEBUG, NCMBUF, "[REC] Recoder has no coded output.\n");
                skb_push_u8(&skb, MAGIC_NUM);
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
                RTE_LOG(DEBUG, NCMBUF,
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

uint8_t decode_udp_data(struct nck_decoder* dec, struct rte_mbuf* m_in,
    struct rte_mempool* mbuf_pool, uint16_t portid,
    void (*put_rxq)(struct rte_mbuf*, uint16_t))
{

        static struct sk_buff skb;
        struct rte_mbuf* m_base = NULL;
        struct rte_mbuf* m_out = NULL;
        uint16_t in_data_len;
        uint16_t in_iphdr_len;
        uint16_t num_decoded = 0;
        struct ipv4_hdr* iph;
        struct udp_hdr* udph;
        uint8_t* pt_data;

        iph = rte_pktmbuf_mtod_offset(m_in, struct ipv4_hdr*, ETHER_HDR_LEN);
        in_iphdr_len = (iph->version_ihl & 0x0F) * 32 / 8;
        udph = (struct udp_hdr*)((char*)iph + in_iphdr_len);
        in_data_len = rte_be_to_cpu_16(udph->dgram_len) - UDP_HDR_LEN;
        pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;

        m_base = mbuf_udp_deep_copy(
            m_in, mbuf_pool, (ETHER_HDR_LEN + in_iphdr_len + UDP_HDR_LEN));

        skb_new(&skb, pt_data, in_data_len);
        skb_put(&skb, in_data_len);
        if (skb_pull_u8(&skb) != MAGIC_NUM) {
                rte_pktmbuf_free(m_in);
                rte_pktmbuf_free(m_base);
                RTE_LOG(DEBUG, NCMBUF,
                    "[DEC] Recv an invalid input. input len: %u\n",
                    in_data_len);
                return 0;
        }
        nck_put_coded(dec, &skb);

        if (!nck_has_source(dec)) {
                rte_pktmbuf_free(m_base);
                rte_pktmbuf_free(m_in);
                RTE_LOG(DEBUG, NCMBUF,
                    "[DEC] Decoder has no output. input len: %u\n",
                    in_data_len);
                return 0;
        }

        while (nck_has_source(dec)) {
                num_decoded += 1;

                if (num_decoded == 1) {
                        skb_new(&skb, pt_data, in_data_len);
                        nck_get_source(dec, &skb);
                        skb.len = rte_be_to_cpu_16(*(uint16_t*)pt_data);
                        pt_data = pt_data + 2;
                        rte_memcpy(
                            pt_data - sizeof(uint16_t), pt_data, skb.len);
                        rte_pktmbuf_trim(m_in, (in_data_len - skb.len));
                        udph->dgram_len
                            = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                        iph->total_length = rte_cpu_to_be_16(
                            skb.len + UDP_HDR_LEN + in_iphdr_len);
                        recalc_cksum(iph, udph);
                        (*put_rxq)(m_in, portid);
                } else {
                        m_out = mbuf_udp_deep_copy(
                            m_base, mbuf_pool, in_iphdr_len + UDP_HDR_LEN);
                        iph = rte_pktmbuf_mtod_offset(
                            m_out, struct ipv4_hdr*, ETHER_HDR_LEN);
                        udph = (struct udp_hdr*)((char*)iph + in_iphdr_len);
                        pt_data = (uint8_t*)(udph) + UDP_HDR_LEN;
                        skb_new(&skb, pt_data, in_data_len);
                        nck_get_source(dec, &skb);
                        skb.len = rte_be_to_cpu_16(*(uint16_t*)(pt_data));
                        pt_data = pt_data + 2;
                        rte_memcpy(
                            pt_data - sizeof(uint16_t), pt_data, skb.len);
                        rte_pktmbuf_trim(m_out, (in_data_len - skb.len));
                        udph->dgram_len
                            = rte_cpu_to_be_16(skb.len + UDP_HDR_LEN);
                        iph->total_length = rte_cpu_to_be_16(
                            skb.len + UDP_HDR_LEN + in_iphdr_len);
                        recalc_cksum(iph, udph);
                        (*put_rxq)(m_out, portid);
                }
                RTE_LOG(DEBUG, NCMBUF,
                    "[DEC] Output num: %u, input len: %u, output len: %u.\n",
                    num_decoded, in_data_len, skb.len);
        }

        rte_pktmbuf_free(m_base);

        return 0;
}
