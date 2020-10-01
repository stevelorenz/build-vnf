#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: STL traffic used for test the scalability of the network functions.
"""

import argparse
import json
import pprint
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

IPv4_HDR_LEN = 20
UDP_HDR_LEN = 8


def init_ports(client):
    all_ports = client.get_all_ports()
    tx_port, rx_port = all_ports
    print(f"TX port: {tx_port}, RX port: {rx_port}")
    client.reset(ports=all_ports)
    return (tx_port, rx_port)


def get_rx_stats(client, tx_port, rx_port):
    pgids = client.get_active_pgids()
    all_stats = client.get_pgid_stats(pgids["latency"])["latency"].get(0)
    error_stats = all_stats["err_cntrs"]
    latency_stats = all_stats["latency"]
    return (error_stats, latency_stats)


def save_rx_stats(ip_tot_len, error_stats, latency_stats):
    with open(f"./error_stats_{ip_tot_len}.data", "a+") as f:
        f.write(json.dumps(error_stats) + "\n")
    with open(f"./latency_stats_{ip_tot_len}.data", "a+") as f:
        f.write(json.dumps(latency_stats) + "\n")


def get_streams(pps, duration, ip_tot_len, ip_src, ip_dst):
    udp_data_len = ip_tot_len - IPv4_HDR_LEN - UDP_HDR_LEN
    if udp_data_len < 16:
        raise RuntimeError("The minimal payload size is 16 bytes.")
    print(f"UDP payload size: {udp_data_len}")
    udp_payload = "Z" * udp_data_len
    pkt = STLPktBuilder(
        pkt=Ether()
        / IP(src=ip_src, dst=ip_dst)
        / UDP(dport=8888, sport=9999, chksum=0)
        / udp_payload
    )
    streams = [
        STLStream(
            name="s0",
            packet=pkt,
            flow_stats=STLFlowLatencyStats(pg_id=0),
            mode=STLTXSingleBurst(pps=pps, total_pkts=int(duration * pps)),
        )
    ]
    return streams


def main():
    parser = argparse.ArgumentParser(
        description="STL traffic used for test the scalability of the network functions."
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
    parser.add_argument("--pps", type=int, default=1e3, help="Packet per second.")
    parser.add_argument("--duration", type=int, default=3, help="Test duration.")
    # Potential lens: 64, 128, 256, 512, 1024
    parser.add_argument("--ip_tot_len", type=int, default=64, help="IP total length")
    parser.add_argument("--num", type=int, default=1, help="Number of rounds tested")

    args = parser.parse_args()
    print(
        "Test information:\n PPS: {}, Duration: {}s, IP_TOT_LEN: {}B, Number of rounds: {}".format(
            args.pps, args.duration, args.ip_tot_len, args.num
        )
    )

    try:
        client = STLClient()
        client.connect()
        tx_port, rx_port = init_ports(client)
        streams = get_streams(
            args.pps, args.duration, args.ip_tot_len, args.ip_src, args.ip_dst
        )

        for n in range(args.num):
            print(f"- Test round: {n+1}")
            client.clear_stats()
            client.reset(ports=[tx_port, rx_port])
            client.add_streams(streams, ports=[tx_port])
            start_ts = time.time()
            client.start(ports=[tx_port], force=True)
            # Wait 3 seconds to let the last packets come back to rx port.
            client.wait_on_traffic(ports=[tx_port], rx_delay_ms=3e3)
            test_dur = time.time() - start_ts
            print(f"Total test duration: {test_dur} seconds.")
            error_stats, latency_stats = get_rx_stats(client, tx_port, rx_port)
            save_rx_stats(args.ip_tot_len, error_stats, latency_stats)

        print("All tests finished! Results are in *.data files")

    except STLError as error:
        print(error)

    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
