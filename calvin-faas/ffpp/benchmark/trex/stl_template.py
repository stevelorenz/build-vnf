#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import json
import os
import pprint
import re
import subprocess
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


def create_streams(stream_params: dict, ip_src: str, ip_dst: str) -> list:
    pass


def create_stream_params(max_bit_rate: float, test: bool) -> list:
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
        "--max_bit_rate",
        type=float,
        default=1,
        help="Maximal bit rate (with the unit Gbps) of the underlying network.",
    )
    parser.add_argument("--test", action="store_true", help="Just used for debug.")

    args = parser.parse_args()

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
