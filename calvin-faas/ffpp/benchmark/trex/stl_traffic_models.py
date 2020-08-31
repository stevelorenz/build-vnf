#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: Test traffic profiles based on MODELS for potential 5G use-cases.
       This is used to generate basic benchmarking results for practical
       use-cases without using data traces of real in-production networks.
       ( so, in short, for papers ;) )

       The focus in on inter-arrival times (has bigger impact on power
       management), so the packet length is fixed for each test.

Reference: - Paper: https://ieeexplore.ieee.org/abstract/document/8985528
           - https://en.wikipedia.org/wiki/Traffic_generation_model#Poisson_traffic_model
           - https://en.wikipedia.org/wiki/Internet_Mix
"""

import argparse
import json
import os
import pprint
import random
import re
import subprocess
import sys
import time

import numpy as np
import stf_path

from trex.stl.api import (
    Ether,
    IP,
    STLClient,
    STLError,
    STLFlowLatencyStats,
    STLFlowStats,
    STLPktBuilder,
    STLProfile,
    STLStream,
    STLTXCont,
    STLTXSingleBurst,
    UDP,
)

# All in bytes.
ETH_PREAMBLE_LEN = 8
ETH_HDR_LEN = 14
ETH_CRC_LEN = 4
ETH_IFG_LEN = 12
ETH_OVERHEAD_LEN_L1 = ETH_PREAMBLE_LEN + ETH_HDR_LEN + ETH_CRC_LEN + ETH_IFG_LEN

IPv4_HDR_LEN = 20
UDP_HDR_LEN = 8

# Fixed PPS for latency monitoring flows.
# 0.1 MPPS, so the resolution is ( 1 / (0.1 * 10 ** 6)) * 10 ** 6  = 10 usec
LATENCY_FLOW_PPS = int(0.01 * 10 ** 6)


def init_ports(client):
    all_ports = client.get_all_ports()
    tx_port, rx_port = all_ports
    print(f"TX port: {tx_port}, RX port: {rx_port}")
    client.reset(ports=all_ports)
    return (tx_port, rx_port)


def get_rx_stats(client, tx_port, rx_port, model, stream_params):
    pass


def save_rx_stats(data, filename, model, stream_params):
    pass


def create_stream_params_poisson(pps, flow_duration, src_num, tot_pkts_burst):
    """ """
    single_burst_time = tot_pkts_burst / pps
    burst_num = int(np.floor(flow_duration / single_burst_time))
    burst_rate = pps / tot_pkts_burst
    # Same as random.expovariate(lmbda)
    intervals = [np.random.exponential(1 / burst_rate) for _ in range(burst_num)]

    print("-----")
    print(flow_duration)
    print(single_burst_time)
    print(burst_num)
    print(burst_rate)
    pprint.pp(list(map(lambda x: x * 1000, intervals)))

    total_duration = 0
    # while True:
    #     pass

    return [[1, 2, 3], [3, 2, 1]]


def create_stream_params(
    bit_rate: float,
    flow_duration: int,
    model: str,
    src_num,
    ip_tot_len: int,
    test: bool,
    tot_pkts_burst=100000,
) -> list:
    """

    Utilize the single burst stream profile in STL to build the model-based traffic.
    The focus here is the inter-arrival times of bursts.
    """
    pps = int(np.floor((bit_rate * 10 ** 9) / ((ip_tot_len + ETH_OVERHEAD_LEN_L1) * 8)))
    pprint.pp(pps)

    # Streams are based
    if model == "Poisson":
        return create_stream_params_poisson(pps, flow_duration, src_num, tot_pkts_burst)
    elif model == "CBR":
        pass
    else:
        raise RuntimeError(f"Unknown model: {model}!")


def get_streams(stream_params):
    pass


def main():

    parser = argparse.ArgumentParser(description="")

    parser.add_argument(
        "--ip_src",
        type=str,
        default="192.168.17.1",
        help="Source IP address for all packets in the stream.",
    )
    parser.add_argument(
        "--ip_dst",
        type=str,
        default="192.168.17.2",
        help="Destination IP address for all packets in the stream.",
    )

    parser.add_argument(
        "--bit_rate", type=float, default=1.0, help="Transmit L1 bit rate in Gbps.",
    )
    parser.add_argument(
        "--ip_tot_len",
        type=int,
        default=1400,
        help="IP packet total length including the IP header.",
    )

    parser.add_argument(
        "--model",
        type=str,
        default="Poisson",
        choices=["Poisson", "CBR"],
        help="To be used traffic model.",
    )
    parser.add_argument(
        "--src_num", type=int, default=1, help="Number of flow sources."
    )
    parser.add_argument(
        "--flow_duration",
        type=int,
        default=10,
        help="The duration of the flow in seconds.",
    )

    parser.add_argument("--test", action="store_true", help="Just used for debug.")

    args = parser.parse_args()

    print(f"* Traffic model: {args.model}")
    stream_params = create_stream_params(
        args.bit_rate,
        args.flow_duration,
        args.model,
        args.src_num,
        args.ip_tot_len,
        args.test,
    )

    if args.test:
        pprint.pp(stream_params)
        sys.exit(0)

    try:
        client = STLClient()
        client.connect()
        tx_port, rx_port = init_ports(client)

        # client.add_streams(streams, ports=[tx_port])

        start_ts = time.time()
        client.clear_stats()
        # All cores in the core_mask is used by the tx_port and its adjacent
        # port, so it is the rx_port normally.
        # client.start(ports=[tx_port], core_mask=[core_mask], force=True)
        client.start(ports=[tx_port], force=True)

    except STLError as error:
        print(error)

    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
