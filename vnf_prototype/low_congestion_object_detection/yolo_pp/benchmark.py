#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Benchmark YOLO Pre-processing VNF
"""

import numpy as np


def print_delays():
    """Print preliminary delay benchmark results"""
    infer_dur = np.loadtxt("./infer_dur.csv")
    rx_buffer_load_dur = np.loadtxt("./rx_buffer_load_dur.csv")
    # microsends
    rx_buffer_send_ts = np.loadtxt("./rx_buffer_send_ts.csv")
    # seconds
    rx_buffer_load_ts = np.loadtxt("./rx_buffer_load_ts.csv")
    print("* Inference duration avg: {} ms, std: {} ms".format(
        np.average(infer_dur) * 1000.0,
        np.std(infer_dur) * 1000.0))

    print("* RX buffer load duration avg: {} ms, std: {} ms".format(
        np.average(rx_buffer_load_dur) * 1000.0,
        np.std(rx_buffer_load_dur) * 1000.0))

    rx_ipc_delay = rx_buffer_load_ts * (10 ** 6) - rx_buffer_send_ts
    print("* RX IPC duration avg: {} ms, std: {} ms".format(
        np.average(rx_ipc_delay) / 1000.0,
        np.std(rx_ipc_delay) / 1000.0))


if __name__ == "__main__":
    print_delays()
