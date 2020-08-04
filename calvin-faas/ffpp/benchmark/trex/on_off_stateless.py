#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: On/Off traffic of a deterministic and stateless stream profile.
       For basic latency benchmark of proposed power management mechanisms.
"""

import argparse
import pprint
import re
import subprocess
import time

import numpy as np
import stf_path
import json

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

PREAMBLE_SIZE = 8  # bytes
# Minimal idle time required to sync clocks before sending the next packet on
# the line.
IFG_SIZE = 12  # bytes
# Full Ethernet frame size including Ether, IP and UDP headers and the payload.
FRAME_SIZE = 512  # bytes

# Fixed PPS for latency monitoring flows.
# 0.1 MPPS, so the resolution is ( 1 / (0.1 * 10 ** 6)) * 10 ** 6  = 10 usec
LATENCY_FLOW_PPS = int(0.01 * 10 ** 6)


def get_core_mask(numa_node: int) -> int:
    outputs = subprocess.run("lscpu", capture_output=True).stdout.decode().splitlines()
    for out in outputs:
        m = re.match(r"NUMA node%d CPU\(s\):\s+((\d,)+\d)" % numa_node, out)
        if m:
            cores = (m.group(1)).split(",")
            break
    else:
        raise RuntimeError(f"Can not get core list of the NUMA node: {numa_node}")

    print("To be used cores:")
    pprint.pp(cores)
    core_mask = sum([2 ** int(c) for c in cores])
    return core_mask


def init_ports(client):
    all_ports = client.get_all_ports()
    tx_port, rx_port = all_ports
    print(f"TX port: {tx_port}, RX port: {rx_port}")
    client.reset(ports=all_ports)
    return (tx_port, rx_port)


def save_rx_stats(data, filename, stream_params):
    wrote_first = False
    with open(filename, "w") as f:
        for index, _ in enumerate(stream_params):
            if not wrote_first:
                f.write("[\n")
                wrote_first = True
            else:
                f.write(",\n")
            json.dump(data[index], f)
        f.write("\n]")


def get_rx_stats(client, tx_port, rx_port, stream_params):
    pgids = client.get_active_pgids()
    print("Currently used pgids: {0}".format(pgids))
    stats = client.get_pgid_stats(pgids["latency"])

    # A list of dictionary
    err_cntrs_results = [dict()] * len(stream_params)
    latency_results = [dict()] * len(stream_params)
    for index, _ in enumerate(stream_params):
        all_stats = stats["latency"].get(index)
        if not all_stats:
            print(f"No stats available for PG ID {index}!")
            continue
        else:
            latency_results[index] = all_stats["latency"]
            err_cntrs_results[index] = all_stats["err_cntrs"]

    return (err_cntrs_results, latency_results)


def create_streams(stream_params: dict, ip_src: str, ip_dst: str) -> list:
    """Create a list of STLStream objects."""

    udp_payload_size = FRAME_SIZE - len(Ether()) - len(IP()) - len(UDP())
    if udp_payload_size < 16:
        raise RuntimeError("The minimal payload size is 16 bytes.")
    print(f"UDP payload size: {udp_payload_size}")
    udp_payload = "Z" * udp_payload_size
    # UDP checksum is disabled.
    pkt = STLPktBuilder(
        pkt=Ether()
        / IP(src=ip_src, dst=ip_dst)
        / UDP(dport=8888, sport=9999, chksum=0)
        / udp_payload
    )

    streams = list()

    # Ref: https://trex-tgn.cisco.com/trex/doc/trex_stateless.html#_tutorial_per_stream_latency_jitter_packet_errors

    # Latency streams are handled fully by software and do not support at full
    # line rate like normal streams. Typically, it is sufficient to have a
    # low-rate latency stream alongside with the real workload stream.
    # In order to have a latency resolution of 1usec, it is not necessary to
    # send a latency stream at a speed higher than 1 MPPS. It is suggested not
    # make the total rate of latency streams higher than 5 MPPS.

    for index, stp in enumerate(stream_params):
        next_st_name = None
        next_st_w_name = None
        self_start = False
        if index != len(stream_params) - 1:
            next_st_name = f"s{index+1}"
            next_st_w_name = f"sw{index+1}"
        if index == 0:
            self_start = True

        # Add extra workload stream.
        workload_stream_pps = stp["pps"] - LATENCY_FLOW_PPS
        streams.append(
            STLStream(
                name=f"sw{index}",
                isg=stp["isg"],
                packet=pkt,
                mode=STLTXSingleBurst(
                    pps=workload_stream_pps,
                    total_pkts=int(workload_stream_pps * stp["on_time"]),
                ),
                next=next_st_w_name,
                self_start=self_start,
            )
        )

        # Add latency monitoring flow with a fixed PPS.
        streams.append(
            STLStream(
                name=f"s{index}",
                isg=stp["isg"],
                packet=pkt,
                flow_stats=STLFlowLatencyStats(pg_id=index),
                mode=STLTXSingleBurst(
                    pps=LATENCY_FLOW_PPS,
                    total_pkts=int(LATENCY_FLOW_PPS * stp["on_time"]),
                ),
                next=next_st_name,
                self_start=self_start,
            )
        )

    return streams


def create_stream_params(
    max_bit_rate: float,
    on_time: int,
    init_off_on_ratio: float,
    iteration: int,
    test: bool,
) -> list:
    """
    Create stream parameters with the link utilization ranging from 0.1 to 1
    with a step of 0.1.
    """

    # MARK: Here pps is the L2 pps, Trex calculates the L1 bps itsel which
    # includes the PREAMBLE_SIZE and IFG_SIZE.
    pps_list = [
        np.floor(
            (utilization * max_bit_rate * 10 ** 9)
            / ((FRAME_SIZE + PREAMBLE_SIZE + IFG_SIZE) * 8)
        )
        for utilization in np.arange(0.1, 1.1, 0.1)
        # for utilization in [1.0] * 10
    ]

    for pps in pps_list:
        if pps < LATENCY_FLOW_PPS:
            raise RuntimeError(
                f"The minimal PPS supported by latency streams is {LATENCY_FLOW_PPS}"
            )

    # In usecs
    # isg_list = [
    #     np.ceil(on_time * init_off_on_ratio * ratio * 10 ** 6)
    #     for ratio in np.arange(1.0, 0, -0.1)
    # ]
    isg_list = [1 * 10 ** 6] * 10

    if test:
        stream_params = [
            {"pps": pps, "isg": isg, "on_time": on_time}
            for pps, isg in zip(pps_list[:3], isg_list[:3])
        ]
    else:
        stream_params = [
            {"pps": pps, "isg": isg, "on_time": on_time}
            for pps, isg in zip(pps_list, isg_list)
        ]

    # Duplicate each stream for iteration numbers.
    stream_params = [s for s in stream_params for _ in range(iteration)]

    return stream_params


def main():

    parser = argparse.ArgumentParser(
        description="On/Off traffic of a deterministic and stateless stream profile."
    )

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
    parser.add_argument("--on_time", type=int, default=2, help="ON time in seconds.")
    parser.add_argument(
        "--init_off_on_ratio",
        type=float,
        default=0.5,
        help="Initial ratio between OFF and ON time.",
    )
    parser.add_argument(
        "--iteration",
        type=int,
        default=1,
        help="Number of iterations for the ON state of each PPS.",
    )
    parser.add_argument(
        "--numa_node",
        type=int,
        default=0,
        help="The NUMA node of cores used for TX and RX.",
    )
    parser.add_argument("--test", action="store_true", help="Just used for debug.")
    parser.add_argument(
        "--out",
        type=str,
        default="",
        help="The name of the output file, stored in /home if given",
    )

    args = parser.parse_args()

    stream_params = create_stream_params(
        args.max_bit_rate,
        args.on_time,
        args.init_off_on_ratio,
        args.iteration,
        args.test,
    )
    print("\n--- To be used stream parameters:")
    pprint.pp(stream_params)
    print()

    core_mask = get_core_mask(args.numa_node)
    print(f"The core mask for RX and TX: {hex(core_mask)}")

    streams = create_streams(stream_params, args.ip_src, args.ip_dst)
    if args.test:
        pprint.pp(streams)
        pprint.pp([s.to_json() for s in streams])

    RX_DELAY_S = sum([s["on_time"] for s in stream_params]) + 3
    RX_DELAY_MS = 1000 * RX_DELAY_S

    try:
        client = STLClient()
        client.connect()
        tx_port, rx_port = init_ports(client)
        client.add_streams(streams, ports=[tx_port])

        # Start TX
        start_ts = time.time()
        client.clear_stats()
        # All cores in the core_mask is used by the tx_port and its adjacent
        # port, so it is the rx_port normally.
        client.start(ports=[tx_port], core_mask=[core_mask], force=True)

        print(f"The estimated RX delay: {RX_DELAY_S} seconds.")
        client.wait_on_traffic(rx_delay_ms=1000 * 3)
        end_ts = time.time()
        test_dur = end_ts - start_ts
        print(f"Total test duration: {test_dur} seconds")

        # Check RX stats.
        # MARK: All latency results are in usec.
        err_cntrs_results, latency_results = get_rx_stats(
            client, tx_port, rx_port, stream_params
        )
        print("--- The latency results of all streams:")
        print(f"- Number of streams: {len(latency_results)}")
        for index, _ in enumerate(stream_params):
            print(f"- Stream: {index}")
            err_cntrs_results[index]["start_ts"] = start_ts
            err_cntrs_results[index]["end_ts"] = end_ts
            print(err_cntrs_results[index])
            print(latency_results[index])
        if args.out:
            savedir = "/home/" + args.out
            print("Results: ", savedir)
            save_rx_stats(err_cntrs_results, 
                        savedir + "_error.json",
                        stream_params)
            save_rx_stats(latency_results, 
                        savedir + "_latency.json",
                        stream_params)

    except STLError as error:
        print(error)

    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
