#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Use virt-copy-out (from libguestfs) to copy measurements data from guest
       VMs to host machine
"""

import os
import shlex
import subprocess


def get_running_domains():
    ret = subprocess.run(shlex.split("virsh list"), stdout=subprocess.PIPE)
    entries = ret.stdout.decode("utf-8").splitlines()[2:-1]  # skip metadata
    domains = [list(filter(None, entry.split(" ")))[1] for entry in entries]
    return domains[:]


def is_csv(f):
    if f.split(".")[-1] == "csv":
        return True
    return False


def get_csv_files(domain, guest_path):
    ret = subprocess.run(
        shlex.split("sudo virt-ls -d {} {}".format(domain, guest_path)),
        stdout=subprocess.PIPE,
    )
    files = ret.stdout.decode("utf-8").splitlines()
    files = list(filter(is_csv, files))
    return files[:]


if __name__ == "__main__":

    guest_path = "/home/ubuntu/dpdk_udp_nc/comp_eval"
    host_path = "./1400B/"
    domains = get_running_domains()
    for domain in domains:
        csv_files = get_csv_files(domain, guest_path)
        for csv in csv_files:
            subprocess.run(
                shlex.split(
                    "sudo virt-copy-out -d {} {} {}".format(
                        domain, os.path.join(guest_path, csv), host_path
                    )
                )
            )
    # Append extra info
    for f in os.listdir(host_path):
        old_f = os.path.join(host_path, f)
        new_f = os.path.join(host_path, "{}_{}".format(len(domains), f))
        os.rename(old_f, new_f)
