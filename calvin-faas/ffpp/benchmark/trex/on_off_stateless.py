#! /usr/bin/env python
# -*- coding: utf-8 -*-

"""
About: On/Off traffic of multiple-dependent stateless streams.
       For basic latency benchmark of proposed power management mechanisms.
"""

import time

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

ETH_HDR_LEN = 14
IP_HDR_LEN = 20

# MARK: Currently hard-coded, maybe auto-generate based on CAIDA datasets
# Streams with name: s0, s1 ... , sn and pgid: 0, 1 ... n
# - pps: Packet-Per-Second (PPS)
# - duration (in seconds): Stream duration. Trex uses total packets to configure the stream
#   duration, so total_pkts = int( pps * duration )
# - isg: Inter-Stream-gap (in microseconds)
STREAM_PROFILES = [
    # This is the initial stream
    {"pps": 1.5 * (10 ** 3), "duration": 2, "isg": 10},
    # Other streams are triggered one-by-one
    {"pps": 3.1 * (10 ** 3), "duration": 2, "isg": 49},
    {"pps": 4.4 * (10 ** 3), "duration": 2, "isg": 22},
    {"pps": 6.1 * (10 ** 3), "duration": 2, "isg": 100},
    {"pps": 7.5 * (10 ** 3), "duration": 2, "isg": 86},
]

RX_DELAY_S = sum([s["duration"] for s in STREAM_PROFILES]) + 3
RX_DELAY_MS = 1000 * RX_DELAY_S

# Full packet size including Ether, IP and UDP headers
PACKET_SIZE = 64


def init_ports(client):
    all_ports = client.get_all_ports()
    tx_port, rx_port = all_ports
    print(f"TX port: {tx_port}, RX port: {rx_port}")
    tx_port_attr = client.get_port_attr(tx_port)
    rx_port_attr = client.get_port_attr(rx_port)
    assert tx_port_attr["src_ipv4"] == "192.168.17.1"
    assert rx_port_attr["src_ipv4"] == "192.168.18.1"
    client.reset(ports=all_ports)
    return (tx_port, rx_port)


def add_streams(client, tx_port):
    udp_payload_size = PACKET_SIZE - len(Ether()) - len(IP()) - len(UDP())
    print(f"UDP payload size: {udp_payload_size}")
    udp_payload = "Z" * udp_payload_size
    pkt = STLPktBuilder(
        pkt=Ether()
        / IP(src="192.168.17.1", dst="192.168.17.2")
        / UDP(dport=8888, sport=9999, chksum=0)
        / udp_payload
    )

    # Add all streams in the STREAM_PROFILES
    streams = list()

    for index, st in enumerate(STREAM_PROFILES[:-1]):
        streams.append(
            STLStream(
                name=f"s{index}",
                isg=st["isg"],
                packet=pkt,
                flow_stats=STLFlowLatencyStats(pg_id=index),
                mode=STLTXSingleBurst(
                    pps=st["pps"], total_pkts=int(st["pps"] * st["duration"])
                ),
                next=f"s{index+1}",
            )
        )

    # Add the last stream without next
    index = len(STREAM_PROFILES) - 1
    st = STREAM_PROFILES[-1]
    streams.append(
        STLStream(
            name=f"s{index}",
            isg=st["isg"],
            packet=pkt,
            flow_stats=STLFlowLatencyStats(pg_id=index),
            mode=STLTXSingleBurst(
                pps=st["pps"], total_pkts=int(st["pps"] * st["duration"])
            ),
        )
    )

    client.add_streams(streams, ports=[tx_port])


def start_tx(client, tx_port):
    client.clear_stats()
    client.start(ports=[tx_port])


def get_rx_stats(client, tx_port, rx_port):
    pgids = client.get_active_pgids()
    print("Currently used pgids: {0}".format(pgids))
    stats = client.get_pgid_stats(pgids["latency"])

    # A list of dictionary
    err_cntrs_results = [dict()] * len(STREAM_PROFILES)
    latency_results = [dict()] * len(STREAM_PROFILES)
    for index, _ in enumerate(STREAM_PROFILES):
        all_stats = stats["latency"].get(index)
        if not all_stats:
            print(f"No stats available for PG ID {index}!")
            continue
        else:
            latency_results[index] = all_stats["latency"]
            err_cntrs_results[index] = all_stats["err_cntrs"]

    return (err_cntrs_results, latency_results)


def main():
    try:
        client = STLClient()
        client.connect()
        tx_port, rx_port = init_ports(client)
        add_streams(client, tx_port)
        start_ts = time.time()
        start_tx(client, tx_port)
        print(f"The estimated RX delay: {RX_DELAY_S} seconds.")
        client.wait_on_traffic(rx_delay_ms=RX_DELAY_MS)
        test_dur = time.time() - start_ts
        print(f"Total test duration: {test_dur} seconds")
        # MARK: All latency results are in usec.
        err_cntrs_results, latency_results = get_rx_stats(client, tx_port, rx_port)
        print("--- The latency results of all streams:")
        for index, _ in enumerate(STREAM_PROFILES):
            print(f"- Stream: {index}")
            print(err_cntrs_results[index])
            print(latency_results[index])

    except STLError as error:
        print(error)

    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
