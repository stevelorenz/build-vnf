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
#include "xdp_util.h"
#endif

#ifndef XDP_ACTION
#define XDP_ACTION REDIRECT
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef UDP_PAYLOAD_LEN
#define UDP_PAYLOAD_LEN 256
#endif

/* Map for redirected port */
BPF_DEVMAP(tx_port, 1);

/* Map of invalid UDP segments */
BPF_PERCPU_ARRAY(udp_nb_map, long, 1);

uint16_t ingress_xdp_redirect(struct xdp_md *ctx)
{
	void *data_end = (void *)(long)ctx->data_end;
	void *data = (void *)(long)ctx->data;

	long *value;
	uint16_t h_proto;
	uint16_t payload_size;
	ACTION action = XDP_ACTION;
	uint64_t nh_off = 0;
	uint32_t key = 0;

	SRC_DST_MAC_INIT

	/* L2 rules */
	struct ethhdr *eth = data;
	nh_off = sizeof(*eth);
	if (data + nh_off > data_end) {
		return XDP_DROP;
	}

	h_proto = eth->h_proto;
	if (h_proto != htons(ETH_P_IP)) {
		return XDP_DROP;
	}

	/* L3 rules */
	struct iphdr *iph = data + nh_off;
	if (data + nh_off + sizeof(*iph) > data_end) {
		return XDP_DROP;
	}
	iph = data + nh_off;
	h_proto = iph->protocol;
	if (h_proto != IPPROTO_UDP) {
		return XDP_DROP;
	}

	/* L4 rules
         * Now check UDP payload length, should equal to a fix number
         * TODO: - Check the magic number in the UDP payload
         *       - Or get payload length info from frontend
         * */
	nh_off = nh_off + 28;
	if (data + nh_off > data_end) {
		return XDP_DROP;
	}
	if (data_end - (data + nh_off) != UDP_PAYLOAD_LEN) {
		if (DEBUG) {
			bpf_trace_printk("[F-OUT-L4] UDP payload length: %lu\n",
					 data_end - (data + nh_off));
		}
		return XDP_DROP;
	}
	value = udp_nb_map.lookup(&key);
	if (value) {
		*value += 1;
	}

	nh_off = 0;
	if (action == BOUNCE) {
		if (rewrite_mac(data, nh_off, data_end, src_mac, dst_mac) ==
		    OPT_FAIL) {
			return XDP_DROP;
		}
		return XDP_TX;
	}

	if (action == REDIRECT) {
		if (rewrite_mac(data, nh_off, data_end, src_mac, dst_mac) ==
		    OPT_FAIL) {
			return XDP_DROP;
		}
		return tx_port.redirect_map(0, 0);
	}

	return XDP_DROP;
}

uint16_t egress_xdp_tx(struct xdp_md *ctx)
{
	return XDP_TX;
}
