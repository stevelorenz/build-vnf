/*
 =====================================================================================
 *
 *       Filename:  xdp_udp_forwarding.c
 *    Description:  Forwarding UDP segments with XDP
 *
 *       Compiler:  llvm
 *          Email:  xianglinks@gmail.com
 *
 =====================================================================================
 */

/*
 * MARK:
 *       - For any bytes modifications, you MUST make sure the bytes you want to
 *       read are within the packet's range BEFORE reading them! Otherwise the
 *       compiler(BPF verifier) throws Permission denied (Invalid access to the
 *       packet).
 * */

#define KBUILD_MODNAME "xdp_udp_forwarding"

#ifndef XDP_UTIL_H
#include "../../../shared_lib/xdp_util.h"
#endif

#ifndef XDP_ACTION
#define XDP_ACTION BOUNCE
#endif

/* Map for TX port */
BPF_DEVMAP(tx_port, 1);
/* Map for number of received packets */
BPF_PERCPU_ARRAY(udp_nb_map, long, 1);

/* UDP forwarding functions */
uint16_t ingress_xdp_redirect(struct xdp_md* ctx)
{
        void* data_end = (void*)(long)ctx->data_end;
        void* data = (void*)(long)ctx->data;

        long* value;
        uint16_t h_proto;
        ACTION action = XDP_ACTION;
        uint64_t nh_off = 0;
        uint32_t key = 0;

        SRC_DST_MAC_INIT

        /* TODO: Filter received Ethernet frame -> Only handle UDP segments */

        value = udp_nb_map.lookup(&key);
        if (value) {
                *value += 1;
        }

#if defined(CKSUM) && (CKSUM == 1)
        /* Recalculate the IP and UDP header checksum */
        nh_off = ETH_HLEN;
        if (udp_cksum(data, nh_off, nh_off + 20, data_end) == OPT_FAIL) {
                return XDP_DROP;
        }
        if (ip_cksum(data, nh_off, data_end) == OPT_FAIL) {
                return XDP_DROP;
        }
#endif

        nh_off = 0;
        if (action == BOUNCE) {
                if (rewrite_mac(data, nh_off, data_end, src_mac, dst_mac)
                    == OPT_FAIL) {
                        return XDP_DROP;
                }
                return XDP_TX;
        }

        if (action == REDIRECT) {
                if (rewrite_mac(data, nh_off, data_end, src_mac, dst_mac)
                    == OPT_FAIL) {
                        return XDP_DROP;
                }
                return tx_port.redirect_map(0, 0);
        }

        return XDP_DROP;
}

uint16_t egress_xdp_tx(struct xdp_md* ctx) { return XDP_TX; }
