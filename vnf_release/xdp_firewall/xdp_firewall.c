/*
 * xdp_firewall.c
 *
 * About: eXpress DataPath based firewall
 *        Compared to common firwall (filter) tools e.g. the iptables
 *
 *        XDP based firewall provides more flexibilities and good performance
 *        at the same time:
 *        - Protocols in application layer can be also handled. For example,
 *        HTTP requests can be parsed to drop requests with invalid or banned
 *        URL before sending to the kernel network stack
 *
 * Email: xianglinks@gmail.com
 *
 */

#define KBUILD_MODNAME "xdp_firewall"

#ifndef XDP_UTIL_H
#include "../../shared_lib/xdp_util.h"
#endif

#ifndef XDP_ACTION
#define XDP_ACTION REDIRECT
#endif

/* Map for redirected port */
BPF_DEVMAP(tx_port, 1);

/* Map of invalid UDP segments */
BPF_PERCPU_ARRAY(udp_nb_map, long, 1);

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

        /* L2 rules */
        h_proto = get_eth_proto(data, nh_off, data_end);

        value = udp_nb_map.lookup(&key);
        if (value) {
                *value += 1;
        }

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
