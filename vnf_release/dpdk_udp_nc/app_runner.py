#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: DPDK UDP NC APP Runner
"""

import argparse
import shlex
import subprocess
import sys

CMD_PREFIX = "sudo"
CMD_SUFFIX = ""
CMD_SUFFIX_EXA = ""

SRC_MAC = "08:00:27:3c:97:68"
DST_MAC = "08:00:27:e1:f1:7d"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="DPDK_UDP_NC APP Runner")
    parser.add_argument("-l", "--lcore", type=int, nargs="+", default=[0],
                        help="a list of to be used lcores.")
    parser.add_argument("-m", "--mem", type=int, default=100,
                        help="memory to allocate in MB")

    parser.add_argument("--app_path", type=str, default="./build/udp_nc",
                        help="the abs path of the APP binary")
    parser.add_argument("--debug", action="store_true",
                        help="run APP with debugging mode")
    parser.add_argument("--kni_mode", action="store_true",
                        help="run APP with KNI mode")

    args = parser.parse_args()

    if args.debug:
        CMD_PREFIX = "sudo gdb --args"
        CMD_SUFFIX_EXA = "--packet-capturing --debugging"

    if args.kni_mode:
        CMD_SUFFIX_EXA = "--kni-mode"

    para_dict = {
        "cmd_prefix": CMD_PREFIX,
        "app_path": args.app_path,
        "lcores": ",".join(map(str, args.lcore)),
        "mem": str(args.mem),
        "src_mac": SRC_MAC,
        "dst_mac": DST_MAC,
        "cmd_suffix": CMD_SUFFIX,
        "cmd_suffix_exa": CMD_SUFFIX_EXA
    }
    print("Configuration parameters: ")
    print(para_dict)

    try:
        subprocess.check_call(
            shlex.split(
                """{cmd_prefix} {app_path} -l {lcores} -m {mem} -- \
                -s {src_mac} -d {dst_mac} \
                -i 10,100,100 \
                -o -1 -b 1 -t 10 \
                -n 2 -p 0x03 -q 2 \
                {cmd_suffix} \
                {cmd_suffix_exa} \
                """.format(**para_dict)
            )
        )
    except KeyboardInterrupt:
        print("[INFO] Exit APP runner")
        sys.exit(0)
