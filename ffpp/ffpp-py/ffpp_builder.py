#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import pprint

from cffi import FFI

ffibuilder = FFI()

ffibuilder.cdef(
    """
    int rte_eal_init(int argc, char **argv);
    int rte_eal_cleanup (void);

    int ffpp_munf_init_manager(struct ffpp_munf_manager *manager,
                            const char *nf_name, struct rte_mempool *pool);
    void ffpp_munf_cleanup_manager(struct ffpp_munf_manager *manager);
"""
)

ffibuilder.set_source(
    "_ffpp_ffi",
    """
    #include <rte_eal.h>
    #include <rte_mempool.h>

    #include <ffpp/munf.h>
""",
    libraries=["rte_eal", "ffpp"],
)


if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
