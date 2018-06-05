/*
 * =====================================================================================
 *
 *       Filename:  packet_filter.c
 *
 *    Description: eBPF program to filter UDP packets from the SFC
 *
 *        Version:  0.0.1
 *		    Email:  xianglinks@gmail.com
 *
 * =====================================================================================
 */

#include <bcc/proto.h>
#include <net/sock.h>
#include <uapi/linux/in.h>
#include <uapi/linux/ptrace.h>

#define ETH_P_IP 0x0800
#define IP_UDP 17

/* Integer presentation of the MAC address of ingress interface */
/* Check https://www.vultr.com/tools/mac-converter/ for MAC converter */
/* Check http://www.aboutmyip.com/AboutMyXApp/IP2Integer.jsp for IP converter */
#define INGRESS_IFCE_SRC_MAC 274973436675894
#define SRC_IP 167772173
#define DST_IP 167772175

/**
 * @brief udp_sfc_filter
 *
 * @param skb
 *
 */
int udp_filter(struct __sk_buff* skb)
{
    u8* cursor = 0;

    struct ethernet_t* eth = cursor_advance(cursor, sizeof(*eth));
    /* ETH_P_IP = 0x0800 */
    if (eth->type != ETH_P_IP) {
	return 0; /* Drop packet */
    }

    /* Check the source MAC address, if this is a loop packet */
    /*bpf_trace_printk("Source MAC: %ld\n", eth->src);*/
    if (eth->src == INGRESS_IFCE_SRC_MAC) {
	bpf_trace_printk(
	    "Receive a frame with src MAC of the ingress interface.\n");
	return 0;
    }

    struct ip_t* ip = cursor_advance(cursor, sizeof(*ip));
    if (ip->nextp != IP_UDP) {
	return 0;
    }

    /* Check source and destination IP */
    if ((ip->src != SRC_IP) || (ip->dst != DST_IP)) {
	bpf_trace_printk("Receive a frame not from the SFC.\n");
	return 0;
    }

    /* Pass the packet to the raw socket */
    return -1;
}
