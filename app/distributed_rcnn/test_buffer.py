#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Test python buffer operations to avoid unnecessary data copies
       Try to utilize Python buffer protocol with bytesarray, memoryview and
       numpy. The numpy array is used by OpenCV.
"""

import multiprocessing
from collections import deque

import cv2 as cv
import memory_profiler
import numpy as np


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

    print("# Test OpenCV methods on numpy.ndarray")
    shape = (432, 320, 3)
    metadata = bytearray(1024)
    buffer = bytearray(65000)
    mv_buffer = memoryview(buffer)
    with open("../../dataset/pedestrian_walking/01-20170320211730-24.jpg", "rb") as f:
        # MARK: There is no offset in the readinto function, then memoryview is
        # used to avoid the temporary copy during readinto
        size = f.readinto(mv_buffer[5:])
    arr = np.frombuffer(buffer, dtype=np.uint8, count=size, offset=5)
    print("Original JPEG image size: {} bytes".format(arr.size))
    # MARK: a new ndarray is allocated to stored decoded image matrix
    frame = cv.imdecode(arr, cv.IMREAD_UNCHANGED)
    frame.resize(shape)
    _, arr = cv.imencode(".jpeg", frame, [cv.IMWRITE_JPEG_QUALITY, 80])
    print("Encoded JPEG image size: {} bytes".format(arr.size))


if __name__ == "__main__":
    test_buffer()
