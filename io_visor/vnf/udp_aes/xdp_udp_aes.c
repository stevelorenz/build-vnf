/*
 =====================================================================================
 *
 *       Filename:  xdp_udp_aes.c
 *    Description:  Encrypt UDP segememts with AES
 *
 *       Compiler:  llvm
 *          Email:  xianglinks@gmail.com
 *
 =====================================================================================
 */

#define KBUILD_MODNAME "xdp_udp_aes"

/* CTR does not need padding */
#define CTR 1
#define CBC 0
#define AES256 1
#ifndef AES_H
#include "../../../shared_lib/aes.h"
#endif

#ifndef XDP_UTIL_H
#include "../../../shared_lib/xdp_util.h"
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

/* The first 16 bytes are used for packet index and timestamp */
#define ENC_OFFSET 16

/* Map for TX port */
BPF_DEVMAP(tx_port, 1);
/* Map for number of received packets */
BPF_PERCPU_ARRAY(udp_nb, long, 1);

uint16_t ingress_xdp_redirect(struct xdp_md* xdp_ctx)
{
        if (DEBUG) {
                bpf_trace_printk("Ingress: Recv Ether\n");
        }
        void* data_end = (void*)(long)xdp_ctx->data_end;
        void* data = (void*)(long)xdp_ctx->data;

        long* value;
        uint16_t h_proto;
        ACTION action = REDIRECT;
        uint64_t nh_off = 0;
        uint32_t key = 0;
        uint8_t drop_flag = 0;

        h_proto = get_eth_proto(data, nh_off, data_end);

        if (h_proto != ETH_P_IP) {
                drop_flag = 1;
                // MARK: ERROR: R1 offset is outside of the packet
                /*goto DROP;*/
        }

        if (DEBUG) {
                bpf_trace_printk("Ingress: Recv UDP segment\n");
        }

        value = udp_nb.lookup(&key);
        if (value) {
                *value += 1;
        }

        /* Encrypt the UDP payload */
        nh_off = UDP_PAYLOAD_OFFSET + ENC_OFFSET;
        uint32_t enc_len = data_end - nh_off;

#ifdef AES256
        uint8_t aes_key[32] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
                0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35,
                0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3,
                0x09, 0x14, 0xdf, 0xf4 };
#endif
        uint8_t aes_iv[16] = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
                0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff };

        struct AES_ctx aes_ctx;
        /* MARK: This part cannot pass the bpf verifier */
        AES_init_ctx_iv(&aes_ctx, aes_key, aes_iv);
        if (DEBUG) {
                bpf_trace_printk("The IV for AES is initialized.\n");
        }
        /*AES_CTR_xcrypt_buffer(&aes_ctx, (data + nh_off), enc_len);*/
        goto DROP;

        nh_off = 0;
        /* MARK: To be used source and destination are hard coded here */
        /*08:00:27:d6:69:61*/
        uint8_t src_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xd6, 0x69, 0x61 };
        /*08:00:27:e1:f1:7d*/
        uint8_t dst_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xe1, 0xf1, 0x7d };
        if (action == BOUNCE) {
                if (rewrite_mac(data, nh_off, data_end, src_mac, dst_mac)
                    == OPT_FAIL) {
                        goto DROP;
                }
                return XDP_TX;
        }

        if (action == REDIRECT) {
                if (rewrite_mac(data, nh_off, data_end, src_mac, dst_mac)
                    == OPT_FAIL) {
                        goto DROP;
                }
                return tx_port.redirect_map(0, 0);
        }
DROP:
        return XDP_DROP;
}

uint16_t egress_xdp_tx(struct xdp_md* ctx) { return XDP_TX; }
