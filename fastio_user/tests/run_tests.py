#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

import argparse
import unittest
from shlex import split
from subprocess import PIPE, run

MEM_CHECK = False
MEM_CHECK_CMD = "valgrind --leak-check=full"

# Default EAL parameters
EAL_PARAS = {
    "cpu": "0",
    "mem": 256,
    "vdev": "eth_af_packet0",
    "iface": "eth0",
    # EAL log level, use 8 for debugging, 1 for info
    "log_level": 1,
    "extra_opts": "--no-pci --single-file-segments"
}


def get_eal_cmd(paras):
    eal_paras_cmd = "-l {cpu} -m {mem} --vdev={vdev},iface={iface} {extra_opts} --log-level=eal,{log_level}".format(
        **paras)
    return eal_paras_cmd


def run_test(cmd):
    if MEM_CHECK:
        cmd = " ".join((MEM_CHECK_CMD, cmd))
    ret = run(split(cmd), check=True, stdout=PIPE, stderr=PIPE)
    output = "\n".join(("- STDOUT:", ret.stdout.decode(), "- STDERR",
                        ret.stderr.decode()))
    return output


def test_ring():
    print("[CASE] Test DPDK ring based communication and mbuf vector operations.")
    paras = EAL_PARAS.copy()
    paras["cpu"] = "0,1"
    eal_cmd = get_eal_cmd(paras)
    cmd = "./test_ring.out {}".format(eal_cmd)
    output = run_test(cmd)
    print(output)


def test_icmp():
    print("[CASE] Test handle ICMP messages.")
    paras = EAL_PARAS.copy()
    paras["cpu"] = "0"
    paras["iface"] = "test_dpdk"
    eal_cmd = get_eal_cmd(paras)
    cmd = "./test_icmp.out {}".format(eal_cmd)
    output = run_test(cmd)
    print(output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Run functionality tests of FastIO_User library.',
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("--case", default="ring",
                        choices=["ring", "icmp"],
                        help="Test case:\n"
                        "\tring: Test DPDK ring based communication and mbuf vector operations.")
    parser.add_argument("--mem_check", action="store_true", default=False)

    args = parser.parse_args()

    if args.mem_check:
        print("* Use valgrind to check memory leaks. WARN: It takes time...")
        MEM_CHECK = True

    dispatcher = {
        "ring": test_ring,
        "icmp": test_icmp,
    }

    dispatcher.get(args.case)()
