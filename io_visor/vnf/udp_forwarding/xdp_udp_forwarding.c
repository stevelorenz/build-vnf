/*
 *
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

#define KBUILD_MODNAME "xdp_udp_forwarding"
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_vlan.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <uapi/linux/bpf.h>

/* Array for number of received packets */
BPF_PERCPU_ARRAY(udp_nb, long, 1);

/* Functions for frame filtering */
static inline uint16_t parse_eth(void* data, uint64_t nh_off, void* data_end)
{
    struct ethhdr* eth = data + nh_off;
    if (data + nh_off > data_end) {
	return XDP_DROP;
    }
    return eth->h_proto;
}

static inline uint16_t parse_ipv4(void* data, uint64_t nh_off, void* data_end)
{
    struct iphdr* iph = data + nh_off;

    if ((void*)&iph[1] > data_end) {
	return XDP_DROP;
    }
    return iph->protocol;
}

/* Functions for header modification */
static inline uint8_t* ether_aton(const uint8_t* asc)
{
    uint8_t ether_addr[ETH_ALEN];
    return ether_addr;
}

/* Permission Denied ... */
static inline void dprint_mac(uint8_t* ether_addr)
{
    bpf_trace_printk("%x:%x:%x:", ether_addr[0], ether_addr[1], ether_addr[2]);
    bpf_trace_printk("%x:%x:%x", ether_addr[3], ether_addr[4], ether_addr[5]);
    bpf_trace_printk("\n");
}

static inline void rewrite_src_dst_mac(void* data, uint64_t nh_off)
{

    /*struct ethhdr {*/
    /*unsigned char	h_dest[ETH_ALEN];	[> destination eth addr	<]*/
    /*unsigned char	h_source[ETH_ALEN];	[> source ether addr	<]*/
    /*__be16		h_proto;		[> packet type ID field	<]*/
    /*} __attribute__((packed));*/
    uint8_t* pt = data;
	uint8_t i = 0;

    /* MARK: To be used source and destination are hard coded here */
    uint8_t src_mac[ETH_ALEN] = { 1, 1, 1, 1, 1, 1 };
    uint8_t dst_mac[ETH_ALEN] = { 2, 2, 2, 2, 2, 2 };

	/* TODO: Find proper method to modify bytes in the frame */
    unsigned short dst[3];
	pt[0] = dst_mac[0];

	/* Destination MAC */
	for (i = 0; i < ETH_ALEN; ++i) {
		/*pt[i] = dst_mac[i];*/
	}
}

/* UDP forwarding function */
int xdp_udp_forwarding(struct xdp_md* ctx)
{
    void* data_end = (void*)(long)ctx->data_end;
    void* data = (void*)(long)ctx->data;

    long* value;
    uint16_t h_proto, ret;
    uint64_t nh_off = 0;
    uint32_t key = 0;
    /*uint16_t debug_flag;*/

    /*debug_flag = data_end - data;*/
    /*bpf_trace_printk("Data length: %d\\n", debug_flag);*/

    /* Filter received Ethernet frame */
    h_proto = parse_eth(data, nh_off, data_end);
    if (h_proto != htons(ETH_P_IP)) {
	XDP_DROP;
    }
    nh_off += sizeof(struct ethhdr);
    h_proto = parse_ipv4(data, nh_off, data_end);
    if (h_proto != htons(IPPROTO_UDP)) {
	XDP_DROP;
    }
    bpf_trace_printk("[DEBUG] Recv a UDP segment.\n");
    /* Increase UDP segment counter */
    value = udp_nb.lookup(&key);
    if (value) {
	*value += 1;
    }

    /* Rewrite MAC addresses */
    nh_off = 0;
    rewrite_src_dst_mac(data, nh_off);

    /* Pass to kernel networking stack -> Used for tcpdump */
    return XDP_PASS;
}
