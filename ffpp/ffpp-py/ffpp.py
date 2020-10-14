#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: Python bindings for FFPP library.
"""

import sys

from _ffpp_ffi import ffi, lib

# EAL


def eal_init(cli_args: list()):
    # Convert Py str to C char* []
    cli_args_ffi = [ffi.new("char[]", arg.encode()) for arg in cli_args]
    ret = lib.rte_eal_init(len(cli_args), cli_args_ffi)
    return ret


def eal_cleanup():
    lib.rte_eal_cleanup()


# FFPP MuNF manager


class MuNFManager(object):
    def __init__(self):
        pass


if __name__ == "__main__":
    print("--- Run ffpp tests")
    cli_args = ["ffpp", "-l", "1"]
    eal_init(cli_args)
    eal_cleanup()
