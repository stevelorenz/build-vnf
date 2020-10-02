#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import pprint

from cffi import FFI

ffibuilder = FFI()

ffibuilder.cdef(
    """
    int rte_eal_init(int argc, char **argv);
    int rte_eal_cleanup (void);
"""
)

ffibuilder.set_source(
    "_pyffpp_ffi",
    """
    #include <rte_eal.h>

""",
    libraries=["rte_eal"],
)


if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
