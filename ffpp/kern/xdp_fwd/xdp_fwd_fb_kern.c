/* SPDX-License-Identifier: GPL-2.0 */
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>

#include <xdp/parsing_helpers.h>

#include <linux/bpf.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include "../common/rewrite_helpers.h"
#include "common_kern_user.h"

#ifndef memcpy
#define memcpy(dest, src, n) __builtin_memcpy((dest), (src), (n))
#endif

struct bpf_map_def SEC("maps") fwd_params_map = {
	.type = BPF_MAP_TYPE_PERCPU_HASH,
	.key_size = ETH_ALEN,
	.value_size = sizeof(struct fwd_params),
	.max_entries = 64,
};

struct bpf_map_def SEC("maps") tx_port = {
	.type = BPF_MAP_TYPE_DEVMAP,
	.key_size = sizeof(int),
	.value_size = sizeof(int),
	.max_entries = 256,
};

struct bpf_map_def SEC("maps") xdp_stats_map = {
	.type = BPF_MAP_TYPE_PERCPU_ARRAY,
	.key_size = sizeof(__u32),
	.value_size = sizeof(struct datarec),
	.max_entries = 1,
};

SEC("xdp_redirect_map")
int xdp_fwd_func(struct xdp_md *ctx)
{
	void *data_end = (void *)(long)ctx->data_end;
	void *data = (void *)(long)ctx->data;
	struct hdr_cursor nh;
	int eth_type;
	struct ethhdr *eth;
	int action = XDP_PASS;

	struct fwd_params *fwd_params;
	int tx_port_key = 0;

	nh.pos = data;
	eth_type = parse_ethhdr(&nh, data_end, &eth);
	if (eth_type < 0) {
		return XDP_ABORTED;
	}

	action = XDP_PASS;

	// MARK: The overhead of map lookup is not negligible.
	fwd_params = bpf_map_lookup_elem(&fwd_params_map, eth->h_source);
	if (!fwd_params) {
		return action;
	}

	// Store the original source and destination MAC
	memcpy(fwd_params->eth_src, eth->h_source, ETH_ALEN);
	memcpy(fwd_params->eth_dst, eth->h_dest, ETH_ALEN);
	// Update source and destination MAC addresses.
	memcpy(eth->h_source, fwd_params->eth_new_src, ETH_ALEN);
	memcpy(eth->h_dest, fwd_params->eth_new_dst, ETH_ALEN);

	tx_port_key = fwd_params->eth_src[ETH_ALEN - 1];

	// Update stats
	__u32 key = 0;
	struct datarec *rec = bpf_map_lookup_elem(&xdp_stats_map, &key);
	if (!rec) {
		return action;
	}
	rec->rx_packets++;

	action = bpf_redirect_map(&tx_port, tx_port_key, 0);
	return action;
}

char _license[] SEC("license") = "GPL";

/* Copied from: $KERNEL/include/uapi/linux/bpf.h
 *
 * User return codes for XDP prog type.
 * A valid XDP program must return one of these defined values. All other
 * return codes are reserved for future use. Unknown return codes will
 * result in packet drops and a warning via bpf_warn_invalid_xdp_action().
 *
enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};

 * user accessible metadata for XDP packet hook
 * new fields must be added to the end of this structure
 *
struct xdp_md {
	// (Note: type __u32 is NOT the real-type)
	__u32 data;
	__u32 data_end;
	__u32 data_meta;
	// Below access go through struct xdp_rxq_info
	__u32 ingress_ifindex; // rxq->dev->ifindex
	__u32 rx_queue_index;  // rxq->queue_index
};
*/
