#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>

#include <linux/bpf.h>

#include <xdp/parsing_helpers.h>

#include "common_kern_user.h"

struct bpf_map_def SEC("maps") xdp_stats_map = {
	.type = BPF_MAP_TYPE_PERCPU_ARRAY,
	.key_size = sizeof(__u32),
	.value_size = sizeof(struct datarec),
	.max_entries = 1,
};

static __always_inline __u32 xdp_stats_record_action(struct xdp_md *ctx,
						     __u32 action,
						     __u64 timestamp)
{
	__u32 key = 0; // Only one action (xdp-pass)

	if (action >= XDP_ACTION_MAX) {
		return XDP_ABORTED;
	}

	struct datarec *rec = bpf_map_lookup_elem(&xdp_stats_map, &key);
	if (!rec) {
		return XDP_ABORTED;
	}

	rec->rx_packets++;
	rec->rx_time = timestamp;

	return action;
}

SEC("xdp_pass")
int xdp_pass_func(struct xdp_md *ctx)
{
	__u32 action = XDP_PASS; /* XDP_PASS = 2 */
	__u64 timestamp = bpf_ktime_get_ns();

	return xdp_stats_record_action(ctx, action, timestamp);
}

// SEC("xdp_drop")
// int xdp_drop_func(struct xdp_md *ctx)
// {
// __u32 action = XDP_DROP;
// __u64 timestamp = bpf_ktime_get_ns();

// return xdp_stats_record_action(ctx, action, timestamp);
// }

// SEC("xdp_abort")
// int xdp_abort_func(struct xdp_md *ctx)
// {
// __u32 action = XDP_ABORTED;
// __u64 timestamp = bpf_ktime_get_ns();

// return xdp_stats_record_action(ctx, action, timestamp);
// }

char _license[] SEC("license") = "GPL";
