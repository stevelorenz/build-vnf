#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Test python buffer operations to avoid unnecessary data copies
       Try to utilize Python buffer protocol with bytesarray, memoryview and
       numpy. The numpy array is used by OpenCV.
"""

from collections import deque

import cv2 as cv
import memory_profiler
import numpy as np
import multiprocessing


@memory_profiler.profile
def test_buffer():

    print("# Test memoryview on bytesarray")
    # MARK: Slicing bytes object creates a additional copy
    a = b"a" * 1024 ** 2 * 10
    b = a[1024:]
    size = 1000
    buffer = bytearray(size)  # bytes array are pre-allocated
    mv_buffer = memoryview(buffer)
    with open("/dev/urandom", "rb") as f:
        f.readinto(mv_buffer)
    frame = np.frombuffer(mv_buffer, dtype=np.uint8)
    frame[0:2] = 10

    q = deque(maxlen=10)
    for i in range(11):
        q.append(i)

    print("# Test memoryview on multiprocessing.queue")


if __name__ == "__main__":
    test_buffer()
