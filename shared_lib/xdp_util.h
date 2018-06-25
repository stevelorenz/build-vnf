/*
 =====================================================================================
 *
 *       Filename:  xdp_util.h
 *    Description:  Util functions for XDP applications
 *          Email:  xianglinks@gmail.com
 *
 =====================================================================================
 */

#ifndef XDP_UTIL_H
#define XDP_UTIL_H

#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <uapi/linux/bpf.h>

/****************
 *  Definition  *
 ****************/

enum OPT_STATE { OPT_SUC = 10, OPT_FAIL = 0 };

enum ACTION { DROP = 0, BOUNCE, REDIRECT };
typedef enum ACTION ACTION;

/**
 * @brief swap_mac: Swap the source and destination MAC addresses
 *      of an Ethernet frame. This is useful for bouncing frames back
 *      with XDP_TX.
 */
static inline uint8_t swap_mac(void* data, uint64_t nh_off, void* data_end);
static inline uint8_t rewrite_mac(void* data, uint64_t nh_off, void* data_end);

/********************
 *  Implementation  *
 ********************/

static inline uint8_t swap_mac(void* data, uint64_t nh_off, void* data_end)
{
        struct ethhdr* eth = data + nh_off;
        if (eth + sizeof(struct ethhdr) > data_end) {
                return XDP_DROP;
        }
        uint8_t tmp[ETH_ALEN];
        __builtin_memcpy(tmp, eth->h_source, ETH_ALEN);
        __builtin_memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
        __builtin_memcpy(eth->h_dest, tmp, ETH_ALEN);
}

static inline uint8_t rewrite_mac(void* data, uint64_t nh_off, void* data_end)
{
        /*struct ethhdr {*/
        /*unsigned char	h_dest[ETH_ALEN];	[> destination eth addr	<]*/
        /*unsigned char	h_source[ETH_ALEN];	[> source ether addr	<]*/
        /*__be16		h_proto;		[> packet type ID field
         * <]*/
        /*} __attribute__((packed));*/
        struct ethhdr* eth = data + nh_off;
        if (eth + sizeof(struct ethhdr) > data_end) {
                return OPT_FAIL;
        }
        /* MARK: To be used source and destination are hard coded here */
        /*08:00:27:d6:69:61*/
        uint8_t src_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xd6, 0x69, 0x61 };
        /*08:00:27:e1:f1:7d*/
        uint8_t dst_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xe1, 0xf1, 0x7d };
        __builtin_memcpy(eth->h_source, src_mac, ETH_ALEN);
        __builtin_memcpy(eth->h_dest, dst_mac, ETH_ALEN);
        return OPT_SUC;
}

#endif /* XDP_UTIL_H! */
