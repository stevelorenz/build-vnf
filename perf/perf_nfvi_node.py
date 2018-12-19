#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Perf the PHYSICAL NFVI node running Linux kernel (eBPF and perf_events
       support is essential)
       This requires the access to the hardware event. Can not work in VM.
       Results are stored in CSV files

Tools:
    - eBPF with bcc frontend
"""

import argparse
import csv
import os
import shlex
import signal
import subprocess
import sys
import time

BCC_TOOL_PATH = ""


def perf_llcstats(duration):
    """Perf the Last Level Cache (LLC) stats"""
    print("# Perf CPU Last Level Cache (LLC)...")
    cmd = shlex.split("sudo {} {}".format(
        os.path.join(BCC_TOOL_PATH, "llcstat"),
        duration
    ))
    ret = subprocess.run(cmd, stdout=subprocess.PIPE)
    entries = ret.stdout.decode('utf-8').splitlines()
    print(entries[0:10])


def main():
    parser = argparse.ArgumentParser(
        description="Perf the PHYSICAL NFVI node running Linux kernel",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--duration", type=int, default=5)
    parser.add_argument("--bcc_tool_path", type=str,
                        default="/usr/share/bcc/tools/")

    args = parser.parse_args()
    global BCC_TOOL_PATH
    BCC_TOOL_PATH = args.bcc_tool_path
    duration = args.duration
    perf_llcstats(duration)


if __name__ == '__main__':
    main()
