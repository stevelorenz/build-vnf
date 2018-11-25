#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
# Copyright © 2018 zuo <zuo@zuo-pc>

"""
About: Test multiprocessing with shared memory
"""

import multiprocessing
import os

from memory_profiler import profile


@profile
def test_fork():
    buffer = bytearray(b"a" * 100)

    children = []
    num_proc = 10
    for _ in range(num_proc):
        pid = os.fork()
        if pid < 0:
            raise RuntimeError("Can not fork a new process")
        elif pid == 0:
            print("This is the child process {}".format(os.getpid()))
            buffer[:] = b"b" * 100
            print("The buffer of current buffer: {}".format(buffer.decode()))
            with open("/dev/null", "wb") as f:
                f.write(buffer)
            os._exit(0)
        else:
            print("This is the parent process {}".format(os.getpid()))
            children.append(pid)

    for i, proc in enumerate(children):
        os.waitpid(proc, 0)

    print("The buffer of the parent: {}".format(buffer.decode()))
    print("The parent process is closing")


if __name__ == "__main__":
    test_fork()
