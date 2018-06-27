/*
 =====================================================================================
 *
 *       Filename:  xdp_udp_xor.c
 *    Description:  XOR UDP payload
 *           MARK:
 *                  - BPF stack limit: max stack of 512 bytes, do not allocate
 *                  too much local variable
 *
 *       Compiler:  llvm
 *          Email:  xianglinks@gmail.com
 *
 =====================================================================================
 */

#define KBUILD_MODNAME "xdp_udp_xor"

#ifndef XDP_UTIL_H
#include "../../../shared_lib/xdp_util.h"
#endif

#ifndef RANDOM_H
#include "../../../shared_lib/random.h"
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef XOR_IFCE
#define XOR_IFCE 0
#endif

/* The first 16 bytes are used for packet index and timestamp */
#define XOR_OFFSET 16
#define MAX_RAND_BYTES_LEN 512

/* Map for TX port */
BPF_DEVMAP(tx_port, 1);

/* Map for number of received packets */
BPF_PERCPU_ARRAY(udp_nb_map, long, 1);

/* Random bytes for XOR
 * Used by the user space program to assign XOR bytes to the XDP program.
 * */
BPF_ARRAY(xor_bytes_map, uint8_t, MAX_RAND_BYTES_LEN);

/* MARK: Do not use global variable, USE maps */
/*static uint8_t XOR_BYTES_ARR[MAX_RAND_BYTES_LEN];*/

/**
 * @brief Initilize the bytes array for XOR
 * @issue CAN not loop over xor_bytes_map
 */
/*static uint8_t init_xor_bytes_arr(uint8_t* xor_bytes_arr, uint16_t len)*/
/*{*/
/*uint32_t key = 0;*/
/*uint8_t* pt_xor_byte;*/
/**/
/*pt_xor_byte = xor_bytes_map.lookup(&key);*/
/*if (pt_xor_byte == NULL) {*/
/*return OPT_FAIL;*/
/*}*/
/*for (key = 0; key < MAX_RAND_BYTES_LEN; ++key) {*/
/*xor_bytes_arr[key] = *(pt_xor_byte);*/
/*}*/
/*return OPT_SUC;*/
/*}*/

/**
 * @brief Handle frames received by ingress interface
 *        - Filter out non-UDP frames
 *        - Rewrite the MAC addresses
 *        - Redirect into the egress interface
 *
 * @param xdp_ctx: XDP context
 *
 */
uint16_t ingress_xdp_redirect(struct xdp_md* xdp_ctx)
{
        if (DEBUG) {
                bpf_trace_printk("[Ingress] Recv an Ether\n");
        }
        void* data_end = (void*)(long)xdp_ctx->data_end;
        void* data = (void*)(long)xdp_ctx->data;

        ACTION action = REDIRECT;
        long* pt_udp_nb = 0;
        uint64_t nh_off = 0;
        uint16_t h_proto = 0;
        uint32_t key = 0; // Used for map lookup and for loops

        uint8_t debug_flag = 0;

        /* TODO: filtering frames */
        h_proto = get_eth_proto(data, nh_off, data_end);
        if (h_proto != ETH_P_IP) {
                // MARK: ERROR: R1 offset is outside of the packet
                /*return XDP_DROP;*/
        }

        if (DEBUG) {
                bpf_trace_printk("[Ingress] Recv a UDP segment\n");
        }

        pt_udp_nb = udp_nb_map.lookup(&key);
        if (pt_udp_nb) {
                *pt_udp_nb += 1;
        }

#if defined(XOR_IFCE) && (XOR_IFCE == 0)
        uint8_t* pt_pload; // Pointer to the UDP payload
        uint8_t* pt_xor_byte;
        /* MARK: Not allowed to use the global variable */
        /* TODO: Use proper method to get XOR bytes from the Map */

        /* XOR the UDP payload */
        nh_off = UDP_PAYLOAD_OFFSET + XOR_OFFSET; // From nh_off -> data_end.
        pt_pload = (uint8_t*)(data + nh_off);

        if (DEBUG) {
                bpf_trace_printk("[DEBUG] nh_off: %u\n,", nh_off);
        }
        /* MARK: Arithmetic on PTR_TO_PACKET_END is prohibited
         * DO NOT use data_end for arithmetic
         * */

        /* [Zuo] Try to walk around the bpf verifier:
         * Nothing serious... I mean really.. Please just run it once...
         * */
        REPEAT_OPT

        /*if ((pt_pload + sizeof(pt_pload) <= data_end)) {*/
                /**pt_pload = (*pt_pload ^ 0x3);*/
                /*pt_pload += 1;*/
        /*}*/

        // for (key = 0; key < 1; ++key) {
        //         if ((pt_pload + sizeof(pt_pload) <= data_end)) {
        //                 *pt_pload = (*pt_pload ^ 0x3);
        //                 pt_pload += 1;
        //         }

        //         else {
        //                 break;
        //         }
        // }
#endif
        /* Recalculate the IP and UDP header checksum */
        nh_off = ETH_HLEN;
        if (ip_cksum(data, nh_off, data_end) == OPT_FAIL) {
                return XDP_DROP;
        }
        nh_off = ETH_HLEN + 20;
        if (udp_cksum(data, nh_off, data_end) == OPT_FAIL) {
                return XDP_DROP;
        }

        if (DEBUG) {
                bpf_trace_printk("[Ingress] Both IP and UDP Cksum OK\n");
        }

        key = 0;
        nh_off = 0;

        /* MARK: To be used source and destination are hard coded here
         */
        uint8_t src_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xd6, 0x69, 0x61 };
        uint8_t dst_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xe1, 0xf1, 0x7d };
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
                if (DEBUG) {
                        bpf_trace_printk("[Ingress] Redirect a UDP segment.\n");
                }
                return tx_port.redirect_map(0, 0);
        }
}

/**
 * @brief Handle frames redirected by the ingress interfaces
 *        - XOR the UDP payload
 *        - Send out
 *
 */
uint16_t egress_xdp_tx(struct xdp_md* xdp_ctx)
{

#if defined(XOR_IFCE) && (XOR_IFCE == 1)
        /* XDP metadata */
        void* data_end = (void*)(long)xdp_ctx->data_end;
        void* data = (void*)(long)xdp_ctx->data;

        if (DEBUG) {
                bpf_trace_printk("[Egress] Recv a frame.\n");
        }

        uint64_t nh_off = 0;
        uint16_t i = 0;
        uint8_t* pt_pload; // Pointer to the UDP payload
        uint8_t* pt_xor_byte;
        /* MARK: Not allowed to use the global variable */
        /* TODO: Use proper method to get XOR bytes from the Map */
        uint8_t xor_bytes_arr[MAX_RAND_BYTES_LEN];
        for (i = 0; i < MAX_RAND_BYTES_LEN; ++i) {
                xor_bytes_arr[i] = 0x3;
        }
        pt_xor_byte = xor_bytes_arr;

        /* XOR the UDP payload */
        nh_off = UDP_PAYLOAD_OFFSET + XOR_OFFSET; // From nh_off -> data_end.
        pt_pload = (uint8_t*)(data + nh_off);
        /* MARK: Arithmetic on PTR_TO_PACKET_END is prohibited
         * DO NOT use data_end for arithmetic
         * */
        // for (i = 0; i < MAX_RAND_BYTES_LEN; ++i) {
        //         if ((pt_pload + sizeof(pt_pload) <= data_end)
        //             && (pt_xor_byte < sizeof(xor_bytes_arr))) {
        //                 *pt_pload = *pt_pload ^ *pt_xor_byte;
        //                 pt_pload += 1;
        //                 pt_xor_byte += 1;
        //         }
        //         break;
        // }
#endif

        return XDP_TX;
}
