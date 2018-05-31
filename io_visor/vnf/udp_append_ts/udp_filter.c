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

    struct ip_t* ip = cursor_advance(cursor, sizeof(*ip));
    if (ip->nextp != IP_UDP) {
	return 0;
    }

    /* TODO: Check if this is a looping packet */

    /* Pass the packet to the raw socket */
    return -1;
}
