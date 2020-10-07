#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Run simple Trex stateless tests using Python automation API.
"""

import argparse
import pprint
import time

import stf_path
import trex_helpers

from trex.stl.api import (
    Ether,
    IP,
    STLClient,
    STLError,
    STLFlowLatencyStats,
    STLPktBuilder,
    STLStream,
    STLTXSingleBurst,
    UDP,
)

PG_ID = 17
TOTAL_PKTS = int(1e7)
PPS = 1e4  # Inter Packet Gap = (1 / PPS)
MONITOR_DUR = 60  # Duration to monitor the flow stats in seconds.
SPOOFED_ETH_SRC = "ab:ab:ab:ab:ab:01"


def rx_interation(c, tx_port, rx_port, total_pkts, pkt_len):
    c.clear_stats()
    c.start(ports=[tx_port])
    pgids = c.get_active_pgids()
    print("Currently used pgids: {0}".format(pgids))

    for _ in range(MONITOR_DUR):
        time.sleep(1)
        # Return flow stats of the given list of pgids.
        stats = c.get_pgid_stats(pgids["latency"])
        flow_stats = stats["flow_stats"].get(PG_ID)
        tx_pps = flow_stats["tx_pps"][tx_port]
        rx_pps = flow_stats["rx_pps"][rx_port]
        tx_pkts = flow_stats["tx_pkts"][tx_port]
        rx_pkts = flow_stats["rx_pkts"][rx_port]
        print(
            f"{time.time()}: tx_pkts: {tx_pkts}, tx_pps: {tx_pps}, rx_pkts:{rx_pkts} ,rx_pps: {rx_pps}"
        )
    # Block until the TX traffic finishes.
    c.wait_on_traffic(ports=[tx_port])

    stats = c.get_pgid_stats(pgids["latency"])
    flow_stats = stats["flow_stats"].get(PG_ID)
    lat_stats = stats["latency"].get(PG_ID)
    if not lat_stats:
        print("No latency stats available!")
        return False

    print("--- Flow and latency stats of the test stream:")
    drops = lat_stats["err_cntrs"]["dropped"]
    ooo = lat_stats["err_cntrs"]["out_of_order"]
    print(f"- Dropped: {drops}, Out-of-Order: {ooo}")
    lat = lat_stats["latency"]
    avg = lat["average"]
    jitter = lat["jitter"]
    hist = lat["histogram"]
    total_max = lat["total_max"]
    print(
        f"- The average latency: {avg} usecs, total max: {total_max} usecs, jitter: {jitter} usecs"
    )
    print("The latency histogram:")
    pprint.pp(hist)

    return True


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description="Run basic stateless test with a single latency stream."
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
        "--total_pkts", type=int, default=int(50), help="Total packets to send."
    )
    parser.add_argument(
        "--pps", type=int, default=int(10), help="Packet-Per-Second (PPS)."
    )
    parser.add_argument(
        "--monitor_dur", type=int, default=5, help="Monitor duration in seconds."
    )
    parser.add_argument(
        "--eth_src_spoofing",
        action="store_true",
        help=f"Replace the Ether source MAC address to {SPOOFED_ETH_SRC}",
    )

    args = parser.parse_args()

    TOTAL_PKTS = args.total_pkts
    PPS = args.pps
    MONITOR_DUR = args.monitor_dur

    print(f"Total TX packets: {TOTAL_PKTS}, PPS: {PPS},Monitor duration: {MONITOR_DUR}")

    # Create a client for stateless tests.
    clt = STLClient()
    passed = True

    try:
        udp_payload = "A" * 1400
        if args.eth_src_spoofing:
            pkt = STLPktBuilder(
                pkt=Ether(SPOOFED_ETH_SRC)
                / IP(src=args.ip_src, dst=args.ip_dst)
                / UDP(dport=8888, sport=9999, chksum=0)
                / udp_payload
            )
        else:
            pkt = STLPktBuilder(
                pkt=Ether()
                / IP(src=args.ip_src, dst=args.ip_dst)
                / UDP(dport=8888, sport=9999, chksum=0)
                / udp_payload
            )
        st = STLStream(
            name="udp_single_burst",
            packet=pkt,
            # Packet group id
            flow_stats=STLFlowLatencyStats(pg_id=PG_ID),
            mode=STLTXSingleBurst(total_pkts=TOTAL_PKTS, pps=PPS),
        )

        clt.connect()
        all_ports = clt.get_all_ports()
        print("All ports: {}".format(",".join(map(str, all_ports))))
        tx_port, rx_port = all_ports
        print(f"TX port: {tx_port}, RX port: {rx_port}")
        tx_port_attr = clt.get_port_attr(tx_port)
        rx_port_attr = clt.get_port_attr(rx_port)
        clt.reset(ports=all_ports)
        clt.add_streams([st], ports=[tx_port])
        print(f"Inject {TOTAL_PKTS} packets on port {all_ports[0]}")

        ret = rx_interation(clt, tx_port, rx_port, TOTAL_PKTS, pkt.get_pkt_len())
        if not ret:
            passed = False

    except STLError as e:
        passed = False
        print(e)

    finally:
        clt.disconnect()

    if passed:
        print("Latency test is passed!")
    else:
        print("Latency test failed!")
