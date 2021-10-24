#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

import subprocess
import argparse
import shlex


def run_test(mps, test_duration, num_rounds, outfile):
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
    parser = argparse.ArgumentParser(description="Script to test network performance with sockperf")
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
    parser.add_argument(
        "-o",
        "--outfile",
        type=str,
        default="result",
        help="Name of result file"
    )
    parser.add_argument(
        "-m",
        "--mps",
        type=int,
        default=10000,
        help="MPS to use for the measurement",
    )
    args = parser.parse_args()

    run_test(args.mps, args.duration, args.num_rounds, args.outfile)

if __name__ == "__main__":
    main()
