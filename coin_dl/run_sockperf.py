#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

import argparse
import json
import shlex
import subprocess


def load_topo_info():
    with open("./share/dumbbell.json", "r", encoding="ascii") as f:
        topo_info = json.load(f)
    return topo_info


def run_test(mps, test_duration, num_rounds, outfile):
    # Calculate the parameters
    topo_info = load_topo_info()

    server_ip = "10.0.3.11"
    sfc_port = 9999
    default_port = 11111
    # TODO: Find the proper mps ?
    # -> just stick with the default of 10000
    for port in [default_port, sfc_port]:
        for round in range(0, num_rounds, 1):
            # TODO: Play with the parameters ?
            cmd = f"sockperf under-load -i {server_ip} -p {port} \
                    --mps {mps} --reply-every 1 \
                    --msg-size 64 \
                    -t {test_duration} \
                    --full-rtt \
                    --full-log ./results/server_local/sockperf/{outfile}_{port}_duration_{test_duration}_mps_{mps}_round_{round}_.log"
            cmd = shlex.split(cmd)
            ret = subprocess.run(cmd, check=True, stdout=subprocess.PIPE)
            assert ret.returncode == 0
            print(ret.stdout.decode("utf-8"))
        # TODO: Parse measurement results and store in ./share for plots
        # -> Not necessary; plotting script handles this for now


def main():
    parser = argparse.ArgumentParser(
        description="Run sockperf to measure network latency."
    )
    parser.add_argument(
        "-d",
        "--duration",
        type=int,
        default=60,
        help="Duration of one iteration",
    )
    parser.add_argument(
        "-r",
        "--num_rounds",
        type=int,
        default=1,
        help="Number of iterations",
    )
    parser.add_argument("--share_dir", default="./share")
    args = parser.parse_args()

    run_test(args.mps, args.duration, args.num_rounds, args.outfile)


if __name__ == "__main__":
    main()
