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

static int write_redirect_params(int map_fd, unsigned char const *src,
				 unsigned char const *dest)
{
	if (bpf_map_update_elem(map_fd, src, dest, 0) < 0) {
		fprintf(stderr,
			"ERR: Failed to update redirect map file with err: %s\n",
			strerror(errno));
		return -1;
	}

	printf("forward: %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x\n",
	       src[0], src[1], src[2], src[3], src[4], src[5], dest[0], dest[1],
	       dest[2], dest[3], dest[4], dest[5]);
	return 0;
}

static int write_eth_new_src_params(int map_fd, unsigned char const *src,
				    unsigned char const *new_src)
{
	if (bpf_map_update_elem(map_fd, src, new_src, 0) < 0) {
		fprintf(stderr,
			"ERR: Failed to update redirect map file with err: %s\n",
			strerror(errno));
		return -1;
	}

	printf("forward: %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x\n",
	       src[0], src[1], src[2], src[3], src[4], src[5], new_src[0],
	       new_src[1], new_src[2], new_src[3], new_src[4], new_src[5]);
	return 0;
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
	char new_src_str[18];
	uint8_t new_src[ETH_ALEN];

	while ((opt = getopt(argc, argv, "hi:r:s:d:w:")) != -1) {
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
			cfg.redirect_ifname = cfg.redirect_ifname_buf;
			break;
		case 's':
			snprintf(cfg.src_mac, 18, "%s", optarg);
			break;
		case 'd':
			snprintf(cfg.dest_mac, 18, "%s", optarg);
			break;
		case 'w':
			snprintf(new_src_str, 18, "%s", optarg);
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
	       new_src_str);

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
	if (parse_mac(new_src_str, new_src) != 0) {
		fprintf(stderr,
			"ERR: Can't parse the to be rewrite new source MAC address: %s\n",
			new_src_str);
	}

	int map_fd;
	map_fd = open_bpf_map_file(pin_dir, "tx_port", NULL);
	if (map_fd < 0) {
		fprintf(stderr, "ERR: Can not open tx_port map.\n");
		return EXIT_FAIL_BPF;
	}

	int i = 0;
	bpf_map_update_elem(map_fd, &i, &cfg.redirect_ifindex, 0);

	map_fd = open_bpf_map_file(pin_dir, "redirect_params", NULL);
	if (map_fd < 0) {
		fprintf(stderr, "ERR: Can not open redirect_params map!\n");
		return EXIT_FAIL_BPF;
	}
	if (write_redirect_params(map_fd, src, dest) < 0) {
		fprintf(stderr, "Can not add redirect parameter.\n");
		return EXIT_FAIL_BPF;
	}

	map_fd = open_bpf_map_file(pin_dir, "eth_new_src_params", NULL);
	if (map_fd < 0) {
		fprintf(stderr, "ERR: Can not open eth_new_src_params map!\n");
		return EXIT_FAIL_BPF;
	}
	if (write_eth_new_src_params(map_fd, src, new_src) < 0) {
		fprintf(stderr, "Can not add new src parameter.\n");
		return EXIT_FAIL_BPF;
	}

	return EXIT_OK;
}
