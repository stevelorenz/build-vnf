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
#include <linux/udp.h>
#include <uapi/linux/bpf.h>

#ifndef CKSUM_H
#include "cksum.h"
#endif

/****************
 *  Definition  *
 ****************/

/* MARK: Assume no IP header options */
#define UDP_PAYLOAD_OFFSET (ETH_HLEN + 20 + 8)

enum OPT_STATE { OPT_SUC = 10, OPT_FAIL = 0 };

enum ACTION { DROP = 0, BOUNCE, REDIRECT };
typedef enum ACTION ACTION;

/**
 * @brief swap_mac: Swap the source and destination MAC addresses
 *      of an Ethernet frame. This is useful for bouncing frames back
 *      with XDP_TX.
 */

static inline uint16_t get_eth_proto(void *data, uint64_t nh_off,
				     void *data_end);
static inline uint16_t get_ip4_proto(void *data, uint64_t nh_off,
				     void *data_end);

static inline uint8_t swap_mac(void *data, uint64_t nh_off, void *data_end);
static inline uint8_t rewrite_mac(void *data, uint64_t nh_off, void *data_end,
				  uint8_t *src_mac, uint8_t *dst_mac);

static inline uint8_t ip_cksum(void *data, uint16_t ip_h_off, void *data_end);
static inline uint8_t udp_cksum(void *data, uint16_t ip_h_off,
				uint16_t udp_h_off, void *data_end);

/********************
 *  Implementation  *
 ********************/

static inline uint16_t get_eth_proto(void *data, uint64_t nh_off,
				     void *data_end)
{
	struct ethhdr *eth = data + nh_off;
	if (data + nh_off > data_end) {
		return OPT_FAIL;
	}
	return eth->h_proto;
}

static inline uint16_t get_ip4_proto(void *data, uint64_t nh_off,
				     void *data_end)
{
	struct iphdr *iph = data + nh_off;
	/* MARK: Do not understand &iph[1] */
	if ((void *)&iph[1] > data_end) {
		return OPT_FAIL;
	}
	return iph->protocol;
}

static inline uint8_t swap_mac(void *data, uint64_t nh_off, void *data_end)
{
	struct ethhdr *eth = data + nh_off;
	if (eth + sizeof(struct ethhdr) > data_end) {
		return XDP_DROP;
	}
	uint8_t tmp[ETH_ALEN];
	__builtin_memcpy(tmp, eth->h_source, ETH_ALEN);
	__builtin_memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
	__builtin_memcpy(eth->h_dest, tmp, ETH_ALEN);
}

static inline uint8_t rewrite_mac(void *data, uint64_t nh_off, void *data_end,
				  uint8_t *src_mac, uint8_t *dst_mac)
{
	/*struct ethhdr {*/
	/*unsigned char	h_dest[ETH_ALEN];	[> destination eth addr	<]*/
	/*unsigned char	h_source[ETH_ALEN];	[> source ether addr	<]*/
	/*__be16		h_proto;		[> packet type ID field
         * <]*/
	/*} __attribute__((packed));*/
	struct ethhdr *eth = data + nh_off;
	if (eth + sizeof(struct ethhdr) > data_end) {
		return OPT_FAIL;
	}
	__builtin_memcpy(eth->h_source, src_mac, ETH_ALEN);
	__builtin_memcpy(eth->h_dest, dst_mac, ETH_ALEN);
	return OPT_SUC;
}

static inline uint8_t ip_cksum(void *data, uint16_t ip_h_off, void *data_end)
{
	struct iphdr *iph = data + ip_h_off;
	if ((void *)&iph[1] > data_end) {
		return OPT_FAIL;
	}
	iph->check = 0;
	iph->check = htons(cksum((uint8_t *)iph, 20, 0));
	return OPT_SUC;
}

static inline uint8_t udp_cksum(void *data, uint16_t ip_h_off,
				uint16_t udp_h_off, void *data_end)
{
	struct iphdr *iph = data + ip_h_off;
	if ((void *)&iph[1] > data_end) {
		return OPT_FAIL;
	}
	iph->check = 0;
	struct udphdr *udph = data + udp_h_off;
	if (udph + sizeof(struct udphdr) > data_end) {
		return OPT_FAIL;
	}
	udph->check = 0;
	/* MARK: generic UDP checksum is disabled because of eBPF verifier */
	// udph->check = htons(cksum((uint8_t*)iph, 272, 1));
	return OPT_SUC;
}

#endif /* XDP_UTIL_H! */
