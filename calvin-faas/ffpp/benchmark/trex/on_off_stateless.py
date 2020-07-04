#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: On/Off traffic of multiple-dependent stateless streams.
       For basic latency benchmark of proposed power management mechanisms.
"""

import argparse
import time

import numpy as np
import stf_path

from trex.stl.api import (
    Ether,
    IP,
    STLClient,
    STLError,
    STLFlowLatencyStats,
    STLPktBuilder,
    STLStream,
    STLTXCont,
    STLTXSingleBurst,
    UDP,
)

PREAMBLE_SIZE = 64  # bits
# Minimal idle time required to sync clocks before sending the next packet on
# the line.
IFG_SIZE = 96  # bits
# Full packet size including Ether, IP and UDP headers
PACKET_SIZE = 64  # bytes


def init_ports(client):
    all_ports = client.get_all_ports()
    tx_port, rx_port = all_ports
    print(f"TX port: {tx_port}, RX port: {rx_port}")
    client.reset(ports=all_ports)
    return (tx_port, rx_port)


def add_streams(client, tx_port, stream_profiles):
    udp_payload_size = PACKET_SIZE - len(Ether()) - len(IP()) - len(UDP())
    print(f"UDP payload size: {udp_payload_size}")
    udp_payload = "Z" * udp_payload_size
    pkt = STLPktBuilder(
        pkt=Ether()
        / IP(src="192.168.17.1", dst="192.168.17.2")
        / UDP(dport=8888, sport=9999, chksum=0)
        / udp_payload
    )

    # Add all streams in the stream_profiles
    streams = list()

    for index, st in enumerate(stream_profiles[:-1]):
        streams.append(
            STLStream(
                name=f"s{index}",
                isg=st["isg"],
                packet=pkt,
                flow_stats=STLFlowLatencyStats(pg_id=index),
                mode=STLTXSingleBurst(
                    pps=st["pps"], total_pkts=int(st["pps"] * st["on_time"])
                ),
                next=f"s{index+1}",
            )
        )

    # Add the last stream without next
    index = len(stream_profiles) - 1
    st = stream_profiles[-1]
    streams.append(
        STLStream(
            name=f"s{index}",
            isg=st["isg"],
            packet=pkt,
            flow_stats=STLFlowLatencyStats(pg_id=index),
            mode=STLTXSingleBurst(
                pps=st["pps"], total_pkts=int(st["pps"] * st["on_time"])
            ),
        )
    )

    client.add_streams(streams, ports=[tx_port])


def start_tx(client, tx_port):
    client.clear_stats()
    client.start(ports=[tx_port])


def get_rx_stats(client, tx_port, rx_port, stream_profiles):
    pgids = client.get_active_pgids()
    print("Currently used pgids: {0}".format(pgids))
    stats = client.get_pgid_stats(pgids["latency"])

    # A list of dictionary
    err_cntrs_results = [dict()] * len(stream_profiles)
    latency_results = [dict()] * len(stream_profiles)
    for index, _ in enumerate(stream_profiles):
        all_stats = stats["latency"].get(index)
        if not all_stats:
            print(f"No stats available for PG ID {index}!")
            continue
        else:
            latency_results[index] = all_stats["latency"]
            err_cntrs_results[index] = all_stats["err_cntrs"]

    return (err_cntrs_results, latency_results)


def generate_stream_profiles(profile, max_bit_rate, on_time, interation):
    """Generate stream profiles with the link utilization ranging from 0.1 to 1
    with a step of 0.1.
    """
    if profile == "deterministic":
        init_off_on_ratio = 0.5
    else:
        raise RuntimeError("Unknown profile!")

    pps_list = [
        np.ceil(
            (utilization * max_bit_rate * 10 ** 9)
            / (PACKET_SIZE * 8 + PREAMBLE_SIZE + IFG_SIZE)
        )
        for utilization in np.arange(0.1, 1.1, 0.1)
    ]
    # In usecs
    isg_list = [
        np.ceil(on_time * init_off_on_ratio * ratio * 10 ** 6)
        for ratio in np.arange(1.0, 0, -0.1)
    ]
    stream_profiles = [
        {"pps": pps, "isg": isg, "on_time": on_time}
        for pps, isg in zip(pps_list, isg_list)
    ]

    # Duplicate each stream for interation numbers.
    stream_profiles = [s for s in stream_profiles for _ in range(interation)]

    return stream_profiles


def main():

    parser = argparse.ArgumentParser(description="ON-OFF stateless traffic generator.")
    parser.add_argument(
        "--max_bit_rate",
        type=float,
        default=1,
        help="Maximal bit rate (with the unit Gbps) of the underlying network.",
    )
    parser.add_argument("--on_time", type=int, default=2, help="ON time in seconds.")
    parser.add_argument(
        "--interation",
        type=int,
        default=1,
        help="Number of interations for each ON state with different PPS",
    )

    args = parser.parse_args()

    stream_profiles = generate_stream_profiles(
        "deterministic", args.max_bit_rate, args.on_time, args.interation
    )
    print("\nTo be used stream profiles:")
    print(stream_profiles)
    print()

    RX_DELAY_S = sum([s["on_time"] for s in stream_profiles]) + 3
    RX_DELAY_MS = 1000 * RX_DELAY_S

    try:
        client = STLClient()
        client.connect()
        tx_port, rx_port = init_ports(client)
        add_streams(client, tx_port, stream_profiles)
        start_ts = time.time()
        start_tx(client, tx_port)
        print(f"The estimated RX delay: {RX_DELAY_S} seconds.")
        client.wait_on_traffic(rx_delay_ms=RX_DELAY_MS)
        test_dur = time.time() - start_ts
        print(f"Total test duration: {test_dur} seconds")
        # MARK: All latency results are in usec.
        err_cntrs_results, latency_results = get_rx_stats(
            client, tx_port, rx_port, stream_profiles
        )
        print("--- The latency results of all streams:")
        print(f"- Number of streams: {len(latency_results)}")
        for index, _ in enumerate(stream_profiles):
            print(f"- Stream: {index}")
            print(err_cntrs_results[index])
            print(latency_results[index])

    except STLError as error:
        print(error)

    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
