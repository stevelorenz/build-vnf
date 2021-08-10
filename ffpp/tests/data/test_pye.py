import os
import time

import numpy as np


def multiply(a, b):
    c = 0
    for i in range(0, a):
        c = c + b
    return c


def handle_int_array(array):
    print(array)
    for i in range(len(array)):
        array[i] += 1
    print(array)
    return 17


def handle_bytes(bytes_in):
    print(type(bytes_in))
    print(bytes_in)
    bytes_out = bytes_in.split(b"_")[0]
    return bytes_out


def handle_bytesarray(barray):
    print(type(barray))
    print(barray)
    return barray.split(b"_")[0]


def print_vector(vector):
    assert type(vector) == list
    print(vector)


if __name__ == "__main__":
    print("Test file path: %s" % os.path.realpath(__file__))
    print("Today is: %s" % time.ctime(time.time()))
    avg = np.average([1, 2, 3])
    print("%.2f" % avg)
    print(multiply(2, 3))
    print("-----------------")
