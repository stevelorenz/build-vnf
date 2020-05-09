#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

import argparse
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
    "extra_opts": "--no-pci --single-file-segments",
}


# Python program to print
# colored text and background
def prRed(skk):
    print("\033[91m {}\033[00m".format(skk))


def prGreen(skk):
    print("\033[92m {}\033[00m".format(skk))


def prYellow(skk):
    print("\033[93m {}\033[00m".format(skk))


def prCyan(skk):
    print("\033[96m {}\033[00m".format(skk))


def get_eal_cmd(paras):
    eal_paras_cmd = "-l {cpu} -m {mem} --vdev={vdev},iface={iface} {extra_opts} --log-level=eal,{log_level}".format(
        **paras
    )
    return eal_paras_cmd


def run_test(name, cmd):
    global ALL_PASS
    if MEM_CHECK:
        cmd = " ".join((MEM_CHECK_CMD, cmd))
    ret = run(split(cmd), check=False, stdout=PIPE, stderr=PIPE)
    if not ret.stderr.decode():
        prYellow(f"[+] Pass {name} test.")
    else:
        ALL_PASS = False
        prRed("[Failed] Naja, sometimes it happens...")
        output = "\n".join(
            ("- STDOUT:", ret.stdout.decode(), "- STDERR", ret.stderr.decode())
        )
        prRed(output)


def test_ring():
    paras = EAL_PARAS.copy()
    paras["cpu"] = "0,1"
    eal_cmd = get_eal_cmd(paras)
    cmd = "./test_ring.out {}".format(eal_cmd)
    run_test("ring", cmd)


def test_all():
    global ALL_PASS
    for func in dispatcher.values():
        func()
    if ALL_PASS:
        prGreen("\n[OK] Pass all tests!!!")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Run functionality tests of FFPP library.",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        "--case",
        default="",
        choices=["ring"],
        help="Test case:\n"
        "\tring: Test DPDK ring based communication and mbuf vector operations.",
    )
    parser.add_argument("--mem_check", action="store_true", default=False)

    args = parser.parse_args()

    if args.mem_check:
        print("* Use valgrind to check memory leaks. WARN: It takes time...")
        MEM_CHECK = True

    ALL_PASS = True

    dispatcher = {"ring": test_ring}

    if not args.case:
        test_all()
    else:
        dispatcher.get(args.case)()
