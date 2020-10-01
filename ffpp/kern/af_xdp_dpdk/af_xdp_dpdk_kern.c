/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/bpf.h>

#include <bpf/bpf_helpers.h>

struct bpf_map_def SEC("maps") xsks_map = {
	.type = BPF_MAP_TYPE_XSKMAP,
	.key_size = sizeof(int),
	.value_size = sizeof(int),
	.max_entries = 64, /* Assume netdev has no more than 64 queues */
};

SEC("xdp_sock")
int xdp_sock_prog(struct xdp_md *ctx)
{
	int ret = 0;
	int index = ctx->rx_queue_index;

	/* A set entry here means that the correspnding queue_id has an active
	 * AF_XDP socket bound to it. */
	ret = bpf_redirect_map(&xsks_map, index, XDP_PASS);
	bpf_printk("RX queue index: %d\n", index);
	bpf_printk("The return value of redirect %d\n", ret);
	if (ret > 0) {
		return ret;
	}

	// Fallback for pre-5.3 kernels, not supporting default action in the
	// flags parameter.
	if (bpf_map_lookup_elem(&xsks_map, &index))
		return bpf_redirect_map(&xsks_map, index, 0);

	bpf_printk("Failed to redirect the frame...\n");
	return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
