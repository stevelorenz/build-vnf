#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: Test traffic profiles based on MODELS for potential 5G use-cases.
       This is used to generate basic benchmarking results for practical
       use-cases without using data traces of real in-production networks.
       ( so, in short, for papers ;) )

       The focus in on inter-arrival times (has bigger impact on power
       management), so the packet length is fixed for each test.

"""

import argparse
import json
import math
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

# Hack to get isgs and ip_tot_lens for json dump
ISGS_SAVE = list()
IP_TOT_LENS_SAVE = list()

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

X_MEN_REACTION_TIME = 30 / (10 ** 3)


def init_ports(client):
    all_ports = client.get_all_ports()
    tx_port, rx_port = all_ports
    print(f"TX port: {tx_port}, RX port: {rx_port}")
    client.reset(ports=all_ports)
    return (tx_port, rx_port)


def save_rx_stats(data, filename, burst_num):
    wrote_first = False
    with open(filename, "w") as f:
        # for index, _ in enumerate(stream_params):
        for index in range(0, burst_num, 1):
            if not wrote_first:
                f.write("[\n")
                wrote_first = True
            else:
                f.write(",\n")
            json.dump(data[index], f)
        f.write("\n]")


def get_rx_stats(client, tx_port, rx_port, burst_num):
    # Check on_off_stateless to store from multiple flows
    pgids = client.get_active_pgids()
    print("Currently used pgids: {0}".format(pgids))
    stats = client.get_pgid_stats(pgids["latency"])

    # A list of dictionary
    err_cntrs = [dict()] * burst_num
    latency = [dict()] * burst_num
    for index in range(0, burst_num, 1):
        lat_stats = stats["latency"].get(index)
        if not lat_stats:
            print(f"No stats available for PG ID {index}!")
            continue
        else:
            latency[index] = lat_stats["latency"]
            err_cntrs[index] = lat_stats["err_cntrs"]

    return(err_cntrs, latency)


def get_ip_tot_len():
    num = random.random()
    if num < 0.58:
        return 64
    elif num < 0.91:
        return 580
    else:
        return 1400


def isgs_sanity_check(isgs):
    avg = np.average(isgs)
    if avg < X_MEN_REACTION_TIME:
        print(f"The average isg: {avg} seconds.")
        raise RuntimeError("There is no time for X-MEN to scale down ;)")


def create_stream_params_poisson(pps, burst_num, src_num, tot_pkts_burst):
    """Create a flow of multiple single burst streams with its inter-arrival
    time in Poisson distribuion.
    """
    global ISGS_SAVE
    global IP_TOT_LENS_SAVE

    single_burst_time = tot_pkts_burst / pps
    burst_rate = pps / tot_pkts_burst
    # Inter-arrival times for each bursts
    isgs = [np.random.exponential(1 / burst_rate) for _ in range(burst_num)]
    ISGS_SAVE = isgs.copy()
    isgs_sanity_check(isgs)
    flow_duration = math.ceil(burst_num * single_burst_time + sum(isgs[:burst_num]))
    # Get packet length for each burst based on IMIX probability.
    ip_tot_lens = [get_ip_tot_len() for _ in range(burst_num)]
    IP_TOT_LENS_SAVE = ip_tot_lens.copy()

    print(f"- Flow duration of {burst_num} bursts: {flow_duration} seconds")

    stream_params = [
        {
            "pps": pps,
            "tot_pkts_burst": tot_pkts_burst,
            "isg": isg,
            "ip_tot_len": ip_tot_len,
        }
        for (isg, ip_tot_len) in zip(isgs, ip_tot_lens)
    ]

    return (stream_params, flow_duration)


def get_streams(
    pps: float,
    burst_num: int,
    model: str,
    src_num,
    tot_pkts_burst,
    l3_data,
    test: bool,
) -> list:
    """
    Utilize the single burst stream profile in STL to build the model-based traffic.
    The ONLY focus here is the inter-arrival times of bursts.
    """

    if src_num > 1:
        raise RuntimeError("Currently not implemented!")
    pps = pps * 10 ** 6
    pprint.pp(pps)
    if pps < LATENCY_FLOW_PPS:
        raise RuntimeError(
            f"The minimal PPS {LATENCY_FLOW_PPS} is required for accuracy."
        )

    # Limit the search symbol table to current module.
    func_params = globals().get(f"create_stream_params_{model}", None)
    if not func_params:
        raise RuntimeError(f"Unknown model: {model}!")
    stream_params, flow_duration = func_params(pps, burst_num, src_num, tot_pkts_burst)

    streams = list()
    for index, param in enumerate(stream_params):
        self_start = False
        next_name = f"s{index+1}"

        if index == 0:
            self_start = True
        elif index == len(stream_params) - 1:
            next_name = None

        udp_payload_len = param["ip_tot_len"] - IPv4_HDR_LEN - UDP_HDR_LEN
        if udp_payload_len < 16:
            raise RuntimeError("The minimal payload size is 16 bytes.")
        udp_payload = "Z" * udp_payload_len
        # UDP checksum is disabled.
        pkt = STLPktBuilder(
            pkt=Ether()
            / IP(src=l3_data["ip_src"], dst=l3_data["ip_dst"])
            / UDP(dport=8888, sport=9999, chksum=0)
            / udp_payload
        )
        streams.append(
            STLStream(
                name=f"s{index}",
                isg=param["isg"] * 10 ** 6,
                packet=pkt,
                flow_stats=STLFlowLatencyStats(pg_id=index),
                mode=STLTXSingleBurst(
                    pps=param["pps"], total_pkts=param["tot_pkts_burst"]
                ),
                next=next_name,
                self_start=self_start,
            ),
        )

    return (streams, flow_duration)


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
        "--bit_rate", type=float, default=3.0, help="Transmit L1 bit rate in Gbps.",
    )

    # Due to different packet sizes, it is easier to keep the PPS fixed.
    # 0.25 Mpps -> about 3Gbps bit rate.
    parser.add_argument(
        "--pps", type=float, default=0.25, help="Transmit L1 rate in Mpps."
    )

    # This is "configured" to give some space for power management ;)
    parser.add_argument(
        "--tot_pkts_burst",
        type=int,
        default=50 * 10 ** 3,
        help="Total number of packets in each single burst.",
    )

    parser.add_argument(
        "--model",
        type=str,
        default="poisson",
        choices=["poisson", "cbr"],
        help="To be used traffic model.",
    )
    # MARK: Currently NOT implemented.
    parser.add_argument(
        "--src_num", type=int, default=1, help="Number of flow sources."
    )

    parser.add_argument(
        "--burst_num",
        type=int,
        default=100,
        help="The number of bursts in one test round.",
    )

    parser.add_argument("--test", action="store_true", help="Just used for debug.")

    parser.add_argument(
        "--out", type=str, default="", help="Stores file with given name",
    )

    args = parser.parse_args()
    print(f"* The fastest reaction time of X-MEN: {X_MEN_REACTION_TIME} seconds.")
    print(f"* Traffic model: {args.model}")

    l3_data = {"ip_src": args.ip_src, "ip_dst": args.ip_dst}
    streams, flow_duration = get_streams(
        args.pps,
        args.burst_num,
        args.model,
        args.src_num,
        args.tot_pkts_burst,
        l3_data,
        args.test,
    )

    if args.test:
        pprint.pp([s.to_json() for s in streams[:3]])

    print(f"* Flow duration: {flow_duration} seconds.")

    try:
        client = STLClient()
        client.connect()
        tx_port, rx_port = init_ports(client)
        client.add_streams(streams, ports=[tx_port])

        start_ts = time.time()
        client.clear_stats()
        client.start(ports=[tx_port], force=True)

        rx_delay_sec = flow_duration + 5
        print(f"The estimated RX delay: {rx_delay_sec} seconds.")
        client.wait_on_traffic(rx_delay_ms=3000)#rx_delay_sec * 10 ** 3)
        end_ts = time.time()
        test_dur = end_ts - start_ts
        print(f"Total test duration: {test_dur} seconds")

        err_cntrs_results, latency_results = get_rx_stats(
            client, tx_port, rx_port, args.burst_num
        )

        print("--- The latency results of all streams:")
        # pprint.pp(err_cntrs_results)
        # pprint.pp(latency_results)
        for m_burst in range(args.burst_num):
            err_cntrs_results[m_burst]["isg"] = ISGS_SAVE[m_burst]
            err_cntrs_results[m_burst]["len"] = IP_TOT_LENS_SAVE[m_burst]
            err_cntrs_results[m_burst]["start_ts"] = start_ts
            err_cntrs_results[m_burst]["end_ts"] = end_ts
            print("Burst ", m_burst)
            print("ISG: ", ISGS_SAVE[m_burst])
            print("Dropped: ", err_cntrs_results[m_burst]["dropped"])
            print("Latency: ", latency_results[m_burst]["average"])
            # print(err_cntrs_results[m_burst])
            # print(latency_results[m_burst])
        if args.out:
            savedir_latency = "/home/malte/malte/latency/"
            savedir_error = "/home/malte/malte/error/"
            if not os.path.exists(savedir_latency):
                os.mkdir(savedir_latency)
            if not os.path.exists(savedir_error):
                os.mkdir(savedir_error)
            savedir_latency += args.out + "_latency.json"
            savedir_error += args.out + "_error.json"
            print("\nResults: ", savedir_latency, ", ", savedir_error)
            save_rx_stats(err_cntrs_results, savedir_error, args.burst_num)
            save_rx_stats(latency_results, savedir_latency, args.burst_num)

    except STLError as error:
        print(error)

    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
