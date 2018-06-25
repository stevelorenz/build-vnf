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

/* Map for TX port */
BPF_DEVMAP(tx_port, 1);
/* Map for number of received packets */
BPF_PERCPU_ARRAY(udp_nb, long, 1);

/* UDP forwarding functions */
uint16_t ingress_xdp_redirect(struct xdp_md* ctx)
{
    void* data_end = (void*)(long)ctx->data_end;
    void* data = (void*)(long)ctx->data;

    long* value;
    uint16_t h_proto;
    /*ACTION action = BOUNCE;*/
    ACTION action = REDIRECT;
    uint64_t nh_off = 0;
    uint32_t key = 0;

    /* MARK: To be used source and destination are hard coded here */
    /*08:00:27:d6:69:61*/
    uint8_t src_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xd6, 0x69, 0x61 };
    /*08:00:27:e1:f1:7d*/
    uint8_t dst_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xe1, 0xf1, 0x7d };

    /* TODO: Filter received Ethernet frame -> Only handle UDP segments */

    value = udp_nb.lookup(&key);
    if (value) {
	*value += 1;
    }
    nh_off = 0;

    if (action == BOUNCE) {
	if (rewrite_mac(data, nh_off, data_end, src_mac, dst_mac) == OPT_FAIL) {
	    return XDP_DROP;
	}
	return XDP_TX;
    }

    if (action == REDIRECT) {
	if (rewrite_mac(data, nh_off, data_end, src_mac, dst_mac) == OPT_FAIL) {
	    return XDP_DROP;
	}
	return tx_port.redirect_map(0, 0);
    }

    return XDP_DROP;
}

uint16_t egress_xdp_tx(struct xdp_md* ctx) { return XDP_TX; }
