#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Generate background/"workload" traffic to make the network nodes busy
"""

import argparse
import json
import os
import shlex
import subprocess


def run(
    topo_params_file, test_duration: int, mps: int, msg_size: int, server_name: str
):
    with open(topo_params_file, "r") as f:
        topo_params = json.load(f)

    server_ip = topo_params["servers"].get(server_name, None)["ip"][:-3]
    port = 11111
    print(f"Run sockperf to send UDP traffic to {server_ip}")
    print(f"Duration: {test_duration} seconds, MPS: {mps}")

    cmd = f"sockperf under-load -i {server_ip} -p {port} \
            --mps {mps} --reply-every 1 \
            --msg-size {msg_size} \
            -t {test_duration} \
            --full-rtt"
    cmd = shlex.split(cmd)
    ret = subprocess.run(cmd, check=True, stdout=subprocess.PIPE)
    assert ret.returncode == 0
    print(ret.stdout.decode("utf-8"))


def main():
    parser = argparse.ArgumentParser(
        description='Run sockperf to generate "workload" traffic to make network nodes busy.'
    )
    parser.add_argument(
        "-d",
        "--duration",
        type=int,
        default=60,
        help="Traffic duration in seconds",
    )

    parser.add_argument(
        "-m",
        "--mps",
        type=int,
        default=10000,
        help="MPS to use for the measurement",
    )
    parser.add_argument(
        "-s",
        "--server",
        type=str,
        default="server1",
        help="The name of the destination server",
    )
    parser.add_argument("--msg_size", type=int, default="1400", help="Message size")
    parser.add_argument(
        "--topo_params_file",
        type=str,
        default="./share/dumbbell.json",
        help="Path of the dumbbell topology JSON file",
    )
    args = parser.parse_args()

    if not os.path.isfile(args.topo_params_file):
        raise RuntimeError(
            f"Can not find the topology JSON file: {args.topo_params_file}"
        )
    run(args.topo_params_file, args.duration, args.mps, args.msg_size, args.server)


if __name__ == "__main__":
    main()
