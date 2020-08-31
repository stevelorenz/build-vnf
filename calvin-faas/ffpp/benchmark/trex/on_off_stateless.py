#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: On/Off traffic of a deterministic and stateless stream profile.
       For basic latency benchmark of proposed power management mechanisms.
"""

import argparse
import os
import pprint
import re
import subprocess
import time

import json
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

# IP total length including IP header
IP_TOT_LEN = 1400  # bytes

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


def get_rx_stats(client, tx_port, rx_port, stream_params, second_stream_params=None):
    err_cntrs_results = list()
    latency_results = list()
    m_idx = 0

    pgids = client.get_active_pgids()
    print("Currently used pgids: {0}".format(pgids))
    stats = client.get_pgid_stats(pgids["latency"])

    # A list of dictionary
    err_cntrs = [dict()] * len(stream_params)
    latency = [dict()] * len(stream_params)
    for index, _ in enumerate(stream_params):
        lat_stats = stats["latency"].get(index)
        # if not all_stats:
        if not lat_stats:
            print(f"No stats available for PG ID {index}!")
            continue
        else:
            latency[index] = lat_stats["latency"]
            err_cntrs[index] = lat_stats["err_cntrs"]
            # m_idx = index
            m_idx += 1

    err_cntrs_results.append(err_cntrs)
    latency_results.append(latency)

    if second_stream_params is not None:
        err_cntrs = [dict()] * len(second_stream_params)
        latency = [dict()] * len(second_stream_params)
        for index, _ in enumerate(second_stream_params):
            lat_stats = stats["latency"].get(index + m_idx)
            if not lat_stats:
                print(f"No stats available for PG ID {index}!")
                continue
            else:
                latency[index] = lat_stats["latency"]
                err_cntrs[index] = lat_stats["err_cntrs"]

        err_cntrs_results.append(err_cntrs)
        latency_results.append(latency)

    # return (err_cntrs_results, latency_results)
    return (err_cntrs_results, latency_results)


def create_streams(stream_params: dict, ip_src: str, ip_dst: str) -> list:
    """Create a list of STLStream objects."""

    udp_payload_size = IP_TOT_LEN - IPv4_HDR_LEN - UDP_HDR_LEN
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


def create_streams_with_second_flow(
    stream_params: dict, second_stream_params: dict, ip_src: str, ip_dst: str
) -> list:
    """Create a list of STLStream objects with the second flow."""

    spoofed_eth_srcs = ["0c:42:a1:51:41:d8", "ab:ab:ab:ab:ab:02"]
    udp_payload_size = IP_TOT_LEN -IPv4_HDR_LEN - UDP_HDR_LEN
    if udp_payload_size < 16:
        raise RuntimeError("The minimal payload size is 16 bytes.")
    print(f"UDP payload size: {udp_payload_size}")
    udp_payload = ["Z" * udp_payload_size, "Z" * (udp_payload_size - 1)]
    # UDP checksum is disabled.

    pkts = list()
    pkts.append(
        STLPktBuilder(
            pkt=Ether(src=spoofed_eth_srcs[0])
            / IP(src=ip_src, dst=ip_dst)
            / UDP(dport=8888, sport=9999, chksum=0)
            / udp_payload[0]
        )
    )
    pkts.append(
        STLPktBuilder(
            pkt=Ether(src=spoofed_eth_srcs[0])
            / IP(src=ip_src, dst=ip_dst)
            / UDP(dport=8888, sport=9999, chksum=0)
            / udp_payload[1]
        )
    )

    streams = list()
    pg_id = 0
    for prefix, params in enumerate([stream_params, second_stream_params]):
        print("Prefix: ", prefix)
        for index, stp in enumerate(params):
            next_st_name = None
            next_st_w_name = None
            self_start = False
            if index != len(params) - 1:
                next_st_name = f"s{prefix+1}{index+1}"
                next_st_w_name = f"sw{prefix+1}{index+1}"
            if index == 0:
                self_start = True

            # Add extra workload stream.
            workload_stream_pps = stp["pps"] - LATENCY_FLOW_PPS
            streams.append(
                STLStream(
                    name=f"sw{prefix+1}{index}",
                    isg=stp["isg"],
                    packet=pkts[prefix],
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
                    name=f"s{prefix+1}{index}",
                    isg=stp["isg"],
                    packet=pkts[prefix],
                    flow_stats=STLFlowLatencyStats(pg_id=pg_id),  # index),
                    mode=STLTXSingleBurst(
                        pps=LATENCY_FLOW_PPS,
                        total_pkts=int(LATENCY_FLOW_PPS * stp["on_time"]),
                    ),
                    next=next_st_name,
                    self_start=self_start,
                )
            )
            pg_id += 1

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
            / ((IP_TOT_LEN + ETH_OVERHEAD_LEN_L1) * 8)
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
    global IP_TOT_LEN

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
        help="The name of the output file, stored in /home/malte/malte/latency if given",
    )
    parser.add_argument(
        "--ip_tot_len",
        type=int,
        default=IP_TOT_LEN,
        help="The IP total length of packets to be transmitted.",
    )
    parser.add_argument(
        "--enable_second_flow",
        action="store_true",
        help="Enable the second flow, used to test two-vnf setup.",
    )

    args = parser.parse_args()

    IP_TOT_LEN = args.ip_tot_len

    stream_params = create_stream_params(
        args.max_bit_rate,
        args.on_time,
        args.init_off_on_ratio,
        args.iteration,
        args.test,
    )
    print("\n--- Initial stream parameters:")
    pprint.pp(stream_params)
    print()

    if args.enable_second_flow:
        print("INFO: The second flow is enabled. Two flows share the physical link.")
        # for s in stream_params:
        # s["pps"] = int(s["pps"] / 2)
        second_stream_params = list(reversed(stream_params.copy()))
        print("\n--- Updated stream parameters with the second flow:")
        pprint.pp(second_stream_params)

    # Does not work on the blackbox
    # core_mask = get_core_mask(args.numa_node)
    # print(f"The core mask for RX and TX: {hex(core_mask)}")

    if args.enable_second_flow:
        streams = create_streams_with_second_flow(
            stream_params, second_stream_params, args.ip_src, args.ip_dst
        )
    else:
        streams = create_streams(stream_params, args.ip_src, args.ip_dst)
        second_stream_params = None

    if args.test:
        pprint.pp([s.to_json() for s in streams])
        import sys

        sys.exit(0)

    if args.enable_second_flow:
        RX_DELAY_S = (sum([s["on_time"] for s in stream_params])) / 2.0 + 3
    else:
        RX_DELAY_S = sum([s["on_time"] for s in stream_params]) + 3

    RX_DELAY_MS = 3 * 1000  # Time after last Tx to wait for the last packet at Rx side

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
        # client.start(ports=[tx_port], core_mask=[core_mask], force=True)
        client.start(ports=[tx_port], force=True)

        print(f"The estimated RX delay: {RX_DELAY_MS / 1000} seconds.")
        client.wait_on_traffic(rx_delay_ms=RX_DELAY_MS)
        end_ts = time.time()
        test_dur = end_ts - start_ts
        print(f"Total test duration: {test_dur} seconds")

        # Check RX stats.
        # MARK: All latency results are in usec.
        # err_cntrs_results, latency_results = get_rx_stats(
        # client, tx_port, rx_port, stream_params
        # )
        err_cntrs_results, latency_results = get_rx_stats(
            client,
            tx_port,
            rx_port,
            stream_params,
            second_stream_params=second_stream_params,
        )
        print("--- The latency results of all streams:")
        print(f"- Number of streams first flow: {len(latency_results[0])}")
        for index, _ in enumerate(stream_params):
            print(f"- Stream: {index}")
            err_cntrs_results[0][index]["start_ts"] = start_ts
            err_cntrs_results[0][index]["end_ts"] = end_ts
            print(err_cntrs_results[0][index])
            print(latency_results[0][index])
        if args.out:
            savedir_latency = "/home/malte/malte/latency/flow1/"
            savedir_error = "/home/malte/malte/error/flow1/"
            if not os.path.exists(savedir_latency):
                os.mkdir(savedir_latency)
            if not os.path.exists(savedir_error):
                os.mkdir(savedir_error)
            savedir_latency += args.out + "_latency.json"
            savedir_error += args.out + "_error.json"
            print("\nResults: ", savedir_latency, ", ", savedir_error)
            save_rx_stats(err_cntrs_results[0], savedir_error, stream_params)
            save_rx_stats(latency_results[0], savedir_latency, stream_params)
        if second_stream_params is not None:
            print(f"\n\n- Number of streams second flow: {len(latency_results[1])}")
            for index, _ in enumerate(stream_params):
                print(f"- Stream: {index}")
                err_cntrs_results[1][index]["start_ts"] = start_ts
                err_cntrs_results[1][index]["end_ts"] = end_ts
                print(err_cntrs_results[1][index])
                print(latency_results[1][index])
            if args.out:
                savedir_latency = "/home/malte/malte/latency/flow2/"
                savedir_error = "/home/malte/malte/error/flow2/"
                if not os.path.exists(savedir_latency):
                    os.mkdir(savedir_latency)
                if not os.path.exists(savedir_error):
                    os.mkdir(savedir_error)
                savedir_latency += args.out + "_latency.json"
                savedir_error += args.out + "_error.json"
                print("\nResults: ", savedir_latency, ", ", savedir_error)
                save_rx_stats(err_cntrs_results[1], savedir_error, stream_params)
                save_rx_stats(latency_results[1], savedir_latency, stream_params)

    except STLError as error:
        print(error)

    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
