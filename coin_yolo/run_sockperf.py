#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

import subprocess
import shlex


def main():
    server_ip = "10.0.3.11"
    sfc_port = 9999
    default_port = 11111
    # TODO: Find the proper mps ?
    mps = 1000
    for port in [default_port, sfc_port]:
        # TODO: Play with the parameters ?
        cmd = f"sockperf under-load -i {server_ip} -p {port} \
                --mps {mps} --reply-every 1 \
                --msg-size 64 \
                -t 10 \
                --full-rtt \
                --full-log ./result_{port}.log"
        cmd = shlex.split(cmd)
        ret = subprocess.run(cmd, check=True, stdout=subprocess.PIPE)
        assert ret.returncode == 0
        print(ret.stdout.decode("utf-8"))
        # TODO: Parse measurement results and store in ./share for plots


if __name__ == "__main__":
    main()
