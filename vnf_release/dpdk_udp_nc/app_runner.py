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
    parser = argparse.ArgumentParser(description="DPDK_UDP_NC APP Runner",
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("-l", "--lcore", type=int, nargs="+", default=[0],
                        help="a list of to be used lcores.")
    parser.add_argument("-m", "--mem", type=int, default=100,
                        help="memory to allocate in MB")
    parser.add_argument("-o", "--operation", type=int, default=-1,
                        help="to be performed operation: \n"
                        "-1: Forwarding \n"
                        " 0: NC encoder \n"
                        " 1: NC decoder \n"
                        " 2: NC recoder \n"
                        )

    parser.add_argument("--burst_num", type=int, default=1,
                        help="the maximum number of packets to retrieve")
    parser.add_argument("--poll_interval", type=str, default="10,100,100",
                        help="polling intervals for the ingress device")
    parser.add_argument("--app_path", type=str, default="./build/udp_nc",
                        help="the abs path of the APP binary")
    parser.add_argument("--src_mac", type=str, default=SRC_MAC,
                        help="source mac address of the original sender")
    parser.add_argument("--dst_mac", type=str, default=DST_MAC,
                        help="destination mac address of the original receiver")

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
        "operation": args.operation,
        "burst_num": args.burst_num,
        "poll_interval": args.poll_interval,
        "cmd_prefix": CMD_PREFIX,
        "app_path": args.app_path,
        "lcores": ",".join(map(str, args.lcore)),
        "mem": str(args.mem),
        "src_mac": args.src_mac,
        "dst_mac": args.dst_mac,
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
                -i {poll_interval} \
                -o {operation} -b {burst_num} -t 10 \
                -n 2 -p 0x03 -q 2 \
                {cmd_suffix} \
                {cmd_suffix_exa} \
                """.format(**para_dict)
            )
        )
    except KeyboardInterrupt:
        print("[INFO] Exit APP runner")
        sys.exit(0)
