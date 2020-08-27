/* SPDX-License-Identifier: GPL-2.0 */

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>
#include <unistd.h>
#include <time.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <net/if.h>
#include <linux/if_link.h> /* depend on kernel-headers installed */
#include <linux/if_ether.h>

#include "../common/common_defines.h"
#include "../common/ext_xdp_user_utils.h"
#include "common_kern_user.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static int parse_u8(char *str, uint8_t *x)
{
	uint32_t z;
	z = strtoul(str, 0, 16);
	if (z > 0xff) {
		return -1;
	}
	if (x) {
		*x = z;
	}
	return 0;
}

static int parse_mac(char *str, uint8_t mac[ETH_ALEN])
{
	uint8_t offset_str = 0;
	uint8_t offset_mac = 0;
	for (offset_str = 0; offset_str < 18; offset_str += 3) {
		if (parse_u8(str + offset_str, &mac[offset_mac]) < 0) {
			return -1;
		}
		offset_mac += 1;
	}
	return 0;
}

void print_fwd_params(const char *ingress_iface_name, const char *eth_src_str,
		      const struct fwd_params *fwd_params)
{
	printf("* Forward packets from interface: %s with source MAC address: %s to interface: %s\n",
	       ingress_iface_name, eth_src_str,
	       fwd_params->redirect_ifname_buf);
	printf("- Updated source MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       fwd_params->eth_new_src[0], fwd_params->eth_new_src[1],
	       fwd_params->eth_new_src[2], fwd_params->eth_new_src[3],
	       fwd_params->eth_new_src[4], fwd_params->eth_new_src[5]);
	printf("- Updated destination MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       fwd_params->eth_dst[0], fwd_params->eth_dst[1],
	       fwd_params->eth_dst[2], fwd_params->eth_dst[3],
	       fwd_params->eth_dst[4], fwd_params->eth_dst[5]);
}

const char *pin_basedir = "/sys/fs/bpf";

void print_usage(void)
{
	printf("Usage: xdp_fwd_user -i <ifname> -r <redirect_ifname> -s <src-mac> -d <dst-mac> -w <new-src-mac>\n");
	printf("Example: xdp_fwd_user -i eth0 -r eth1 -s 02:42:ac:11:00:02 -d 02:42:ac:11:00:03 -w 02:42:ac:11:00:01\n");
}

int main(int argc, char *argv[])
{
	int opt = 0;
	struct config cfg = { 0 };
	uint8_t eth_new_src[ETH_ALEN] = { 0 };
	char eth_new_src_str[18] = { 0 };

	struct fwd_params fwd_params = { 0 };

	char *ptr; // dummy pointer for strtoul
	int payload_size = -1;

	while ((opt = getopt(argc, argv, "hi:r:s:d:w:p:")) != -1) {
		switch (opt) {
		case 'h':
			print_usage();
			return EXIT_OK;
		case 'i':
			cfg.ifindex = if_nametoindex(optarg);
			snprintf(cfg.ifname_buf, IF_NAMESIZE, "%s", optarg);
			cfg.ifname = cfg.ifname_buf;
			break;
		case 'r':
			cfg.redirect_ifindex = if_nametoindex(optarg);
			snprintf(cfg.redirect_ifname_buf, IF_NAMESIZE, "%s",
				 optarg);
			snprintf(fwd_params.redirect_ifname_buf, IF_NAMESIZE,
				 "%s", optarg);
			cfg.redirect_ifname = cfg.redirect_ifname_buf;
			break;
		case 's':
			snprintf(cfg.src_mac, 18, "%s", optarg);
			break;
		case 'd':
			snprintf(cfg.dest_mac, 18, "%s", optarg);
			break;
		case 'w':
			snprintf(eth_new_src_str, 18, "%s", optarg);
			break;
		case 'p':
			payload_size = strtoul(optarg, &ptr, 10);
			break;
		case 0:
			break;
		default:
			print_usage();
			return EXIT_FAIL_OPTION;
		}
	}
	printf("Redirect traffic from interface: %s to interface: %s.\n",
	       cfg.ifname_buf, cfg.redirect_ifname_buf);
	printf("Source MAC: %s, Destination MAC: %s\n", cfg.src_mac,
	       cfg.dest_mac);
	printf("The source MAC will be rewrited to new address: %s\n",
	       eth_new_src_str);

	int len;
	char pin_dir[PATH_MAX];
	len = snprintf(pin_dir, PATH_MAX, "%s/%s", pin_basedir, cfg.ifname);
	if (len < 0) {
		fprintf(stderr, "ERR: creating pin dirname\n");
		return EXIT_FAIL_OPTION;
	}

	uint8_t src[ETH_ALEN];
	uint8_t dest[ETH_ALEN];

	if (parse_mac(cfg.src_mac, src) != 0) {
		fprintf(stderr, "ERR: Can't parse source MAC address: %s\n",
			cfg.src_mac);
	}
	if (parse_mac(cfg.dest_mac, dest) != 0) {
		fprintf(stderr,
			"ERR: Can't parse destination MAC address: %s\n",
			cfg.dest_mac);
	}
	memcpy(fwd_params.eth_new_dst, dest, ETH_ALEN * sizeof(uint8_t));
	if (parse_mac(eth_new_src_str, eth_new_src) != 0) {
		fprintf(stderr,
			"ERR: Can't parse the to be rewrite new source MAC address: %s\n",
			eth_new_src_str);
	}
	memcpy(fwd_params.eth_new_src, eth_new_src, ETH_ALEN * sizeof(uint8_t));

	int map_fd;
	map_fd = open_bpf_map_file(pin_dir, "tx_port", NULL);
	if (map_fd < 0) {
		fprintf(stderr, "ERR: Can not open tx_port map.\n");
		return EXIT_FAIL_BPF;
	}

	if (payload_size < 0) {
		// Use the last byte of the source MAC address as the key.
		int i = 0;
		i = src[ETH_ALEN - 1];
		bpf_map_update_elem(map_fd, &i, &cfg.redirect_ifindex, 0);
	} else {
		// Use payload size of the packet as key
		payload_size = payload_size & 0x000000FF;
		bpf_map_update_elem(map_fd, &payload_size,
				    &cfg.redirect_ifindex, 0);
	}

	map_fd = open_bpf_map_file(pin_dir, "fwd_params_map", NULL);
	if (map_fd < 0) {
		fprintf(stderr, "ERR: Can not open fwd_params_map!\n");
		return EXIT_FAIL_BPF;
	}
	if (bpf_map_update_elem(map_fd, src, &fwd_params, 0) < 0) {
		fprintf(stderr,
			"ERR: Failed to update fwd_params_map file with err: %s\n",
			strerror(errno));
		return -1;
	}
	print_fwd_params(cfg.ifname, cfg.src_mac, &fwd_params);

	return EXIT_OK;
}
