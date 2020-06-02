/* SPDX-License-Identifier: GPL-2.0
 *
 * About: The loader for xdp_time_kern.o
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <locale.h>
#include <unistd.h>
#include <time.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <net/if.h>
#include <linux/if_link.h> /* depend on kernel-headers installed */

#include "../common/common_defines.h"
#include "../common/ext_xdp_user_utils.h"
#include "common_kern_user.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const char *pin_basedir = "/sys/fs/bpf";
static const char *map_name = "xdp_stats_map";
static const char *default_filename = "xdp_time_kern.o";

int pin_maps_in_bpf_object(struct bpf_object *bpf_obj, const char *subdir)
{
	char map_filename[PATH_MAX];
	char pin_dir[PATH_MAX];
	int err, len;

	len = snprintf(pin_dir, PATH_MAX, "%s/%s", pin_basedir, subdir);
	if (len < 0) {
		fprintf(stderr, "ERR: creating pin dirname\n");
		return EXIT_FAIL_OPTION;
	}
	len = snprintf(map_filename, PATH_MAX, "%s/%s/%s", pin_basedir, subdir,
		       map_name);
	if (len < 0) {
		fprintf(stderr, "ERR: creating map_name\n");
		return EXIT_FAIL_OPTION;
	}

	if (access(map_filename, F_OK) != -1) {
		printf("- Unpinning prev maps in %s\n", pin_dir);
		err = bpf_object__unpin_maps(bpf_obj, pin_dir);
		if (err) {
			fprintf(stderr, "ERR: Unpinging maps in %s\n", pin_dir);
			return EXIT_FAIL_BPF;
		}
	}
	printf("- Pinning maps in %s\n", pin_dir);

	err = bpf_object__pin_maps(bpf_obj, pin_dir);
	if (err) {
		fprintf(stderr, "ERR: Pinning maps in %s\n", pin_dir);
		return EXIT_FAIL_BPF;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr,
			"ERR: Invalid option! Missing interface name.\n");
		fprintf(stdout, "Usage: xdp_time_loader <ifname>\n");
		return EXIT_FAIL_OPTION;
	}

	struct config cfg = {
		// Use XDP native mode
		// For skb mode, use XDP_FLAGS_SKB_MODE instead.
		// For hardware offloading, use XDP_FLAGS_HW_MODE instead.
		.xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST | XDP_FLAGS_DRV_MODE,
		.ifindex = -1,
		.do_unload = false,
		.ifname = argv[1],
	};

	snprintf(cfg.filename, sizeof(cfg.filename), "%s", default_filename);
	cfg.ifindex = if_nametoindex(cfg.ifname);
	if (cfg.ifindex <= 0) {
		fprintf(stderr, "ERR: Can not find interface: %s\n",
			cfg.ifname);
		return EXIT_FAIL_OPTION;
	}

	struct bpf_object *bpf_obj;
	bpf_obj = load_bpf_and_xdp_attach(&cfg);
	if (!bpf_obj) {
		fprintf(stderr, "ERR: Can not attach the XDP program.");
		return EXIT_FAIL_BPF;
	}
	printf("Success: Loaded BPF-object(%s)\n", cfg.filename);
	printf("- XDP attached on device:%s(ifindex:%d)\n", cfg.ifname,
	       cfg.ifindex);

	/* Export and pin the maps */
	int err = 0;
	err = pin_maps_in_bpf_object(bpf_obj, cfg.ifname);
	if (err) {
		fprintf(stderr, "ERR: pinning maps\n");
		return err;
	}

	return EXIT_OK;
}
