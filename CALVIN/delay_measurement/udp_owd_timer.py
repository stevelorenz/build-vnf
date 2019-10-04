#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: UDP One-Way Delay (OWD) timer

       - Packets are sent via layer 4 socket (AF_INET, SOCK_DGRAM)
       - Use synchronous and blocking IO
       - The server CAN also calculate the bandwidth. Only used for tests.

Email: xianglinks@gmail.com
"""

import argparse
import logging
import os
import signal
import socket
import struct
import sys
import time

MAX_PAYLOAD_SIZE = 512
MAX_SEND_SLOW_NUMBER = 10


def run_client():
    """Run UDP client"""
    send_slow_nb = 0
    send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if BIND_SRC_ADDR:
        send_sock.bind((SRC_ADDR[0], SRC_ADDR[1]))
    send_idx = 0  # uint64_t
    data_arr = bytearray(MAX_PAYLOAD_SIZE)
    # MARK: First 16 bytes: send_idx(uint64_t) + time_stamp(uint64_t)
    data_arr[16:PAYLOAD_SIZE] = b"a" * (PAYLOAD_SIZE - 16)

    try:
        while True:
            # MARK: uint64_t is used for timestamps in the VNF
            data_arr[0:16] = struct.pack(
                "!QQ",
                send_idx,
                # Get time stamp in microseconds.
                int(time.time() * 1e6),
            )
            st_send_ts = time.time()
            send_sock.sendto(data_arr[0:PAYLOAD_SIZE], SRV_ADDR)
            send_dur = time.time() - st_send_ts
            if IPD - send_dur > 0:
                time.sleep(IPD - send_dur)
                send_slow_nb = 0
            else:
                send_slow_nb += 1
                if send_slow_nb >= MAX_SEND_SLOW_NUMBER:
                    raise Exception("Client sends too slow, the IDP may be too small.")

            logger.debug(
                "Packet Idx: %d, Send duration: %f, sleep time: %f, payload: %s",
                send_idx,
                send_dur,
                IPD - send_dur,
                data_arr,
            )
            send_idx += 1

            if send_idx > int(2e18 - 1):
                raise RuntimeError("Send index out of range.")

            if CLIENT_MODE == 0:
                if send_idx == N_PACKETS:
                    break

    except KeyboardInterrupt:
        logger.info("KeyboardInterrupt detected, exit.")
        send_sock.close()


def run_server(bw, payload_size):
    """Run UDP server"""

    recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    recv_sock.bind(SRV_ADDR)
    recv_sock.settimeout(None)

    csv_file = open(OUTPUT_FILE, "a+")

    def exit_server(*args):
        logger.debug("SIGTERM detected, save all data in the buffer and exit.")
        recv_sock.close()
        # Sync buffered data to the disk
        csv_file.flush()
        os.fsync(csv_file.fileno())
        csv_file.close()
        sys.exit(0)

    signal.signal(signal.SIGTERM, exit_server)

    owd_result = list()
    data_buffer = list()
    bw_mon_st = 0
    bw_mon_end = 0

    try:
        for rd in range(1, ROUND + 1):
            recv_idx = 0
            owd_result.clear()
            data_buffer.clear()
            logger.info("[Server] Current test round: %d" % rd)
            logger.info(
                "[Server] Try to receive %d packets... MARK: No check for the ordering during receiving"
                % N_PACKETS
            )
            while recv_idx < N_PACKETS:
                # MARK: Problem! If the calculation takes too much time, the
                # server may not able to receive all incoming packets
                # So now first try to receiving as fast as possible and
                # calculate latter...
                data = recv_sock.recv(SRV_BUFFER_SIZE)
                if recv_idx == 0:
                    bw_mon_st = time.time()
                recv_ts = int(time.time() * 1e6)
                data_buffer.append((data, recv_ts))
                recv_idx += 1
            else:
                bw_mon_end = time.time()
                # Check if receive all packets
                if len(data_buffer) != N_PACKETS:
                    raise Exception("[Server] Not all packets are received.")
                if bw:
                    logger.info(
                        "[Server] Finish receiving packets, check ordering and calculate bandwidth"
                    )
                    for recv_idx, (data, recv_ts) in enumerate(data_buffer):
                        send_idx, send_ts = struct.unpack("!QQ", data[0:16])
                        if recv_idx != send_idx:
                            raise Exception(
                                "The packet order is not right!, send_idx:%d, recv_idx:%d"
                                % (send_idx, recv_idx)
                            )
                    # bytes/second
                    bw = ((N_PACKETS - 1) * payload_size) / (bw_mon_end - bw_mon_st)
                    # Mbits/second
                    bw = bw * 8 / (10.0 ** 6)
                    csv_file.write(str(bw))
                    csv_file.write("\n")
                else:
                    logger.info(
                        "[Server] Finish receiving packets, check ordering and calculate OWDs"
                    )
                    for recv_idx, (data, recv_ts) in enumerate(data_buffer):
                        send_idx, send_ts = struct.unpack("!QQ", data[0:16])
                        if recv_idx != send_idx:
                            raise Exception(
                                "The packet order is not right!, send_idx:%d, recv_idx:%d"
                                % (send_idx, recv_idx)
                            )
                        owd = recv_ts - send_ts
                        owd_result.append(owd)

                    csv_file.write(",".join(map(str, owd_result)))
                    csv_file.write("\n")

    except Exception as e:
        csv_file.write(str(e))
        csv_file.write("\n")

    finally:
        recv_sock.close()
        csv_file.close()


if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description="Timer for UDP one way delay.",
        formatter_class=argparse.RawTextHelpFormatter,
    )

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        "-s", "--server", metavar="Address", type=str, help="Run in UDP server mode"
    )
    parser.add_argument(
        "-o",
        "--output_file",
        type=str,
        default="udp_owd.csv",
        help="Output file of test result",
    )
    parser.add_argument(
        "--srv_buffer",
        type=int,
        default=512,
        metavar="Buffer Size",
        help="Server recv buffer size in bytes",
    )
    parser.add_argument(
        "-r", "--round", type=int, default=1, help="Number of sending rounds"
    )
    parser.add_argument(
        "--bw",
        action="store_true",
        help="Measure bandwidth instead of OWD on the server side",
    )

    group.add_argument(
        "-c", "--client", metavar="Address", type=str, help="Run in UDP client mode"
    )
    parser.add_argument(
        "-b",
        "--bind",
        metavar="Address",
        type=str,
        default="",
        help="Bind client to a specific address (IP:PORT)",
    )
    parser.add_argument(
        "--payload_size", type=int, default=512, help="UDP payload size in bytes"
    )
    parser.add_argument(
        "--ipd", type=float, default=1.0, help="Inter-packet delay in seconds"
    )

    parser.add_argument(
        "-m",
        type=int,
        default=0,
        help="""Client send mode\n
\t 0: Send n_packets number of packets and stop.\n
\t 1: Keep sending until interrupt.\n
                        """,
    )

    parser.add_argument(
        "-n", "--n_packets", type=int, help="Number of sent packets", default=10
    )
    parser.add_argument(
        "-l", "--log_level", type=str, help="Logging level", default="INFO"
    )

    args = parser.parse_args()

    fmt_str = "%(asctime)s %(levelname)-8s %(message)s"
    level = {"INFO": logging.INFO, "DEBUG": logging.DEBUG, "ERROR": logging.ERROR}
    logger = logging.getLogger(__name__)

    logging.basicConfig(
        level=args.log_level, handlers=[logging.StreamHandler()], format=fmt_str
    )

    N_PACKETS = args.n_packets
    SRV_BUFFER_SIZE = args.srv_buffer
    ROUND = args.round

    if args.server:
        ip, port = args.server.split(":")
        SRV_ADDR = (ip, int(port))
        OUTPUT_FILE = args.output_file
        logger.info("Run UDP server listening on %s:%s.", ip, port)
        if args.bw:
            logger.info("Measure bandwidth instead of OWD.")
        PAYLOAD_SIZE = args.payload_size
        run_server(args.bw, PAYLOAD_SIZE)

    if args.client:
        BIND_SRC_ADDR = False
        ip, port = args.client.split(":")
        SRV_ADDR = (ip, int(port))
        SRC_ADDR = ("all", 0)
        if args.bind:
            BIND_SRC_ADDR = True
            ip, port = args.bind.split(":")
            SRC_ADDR = (ip, int(port))
        IPD = args.ipd
        PAYLOAD_SIZE = args.payload_size
        CLIENT_MODE = args.m
        logger.info(
            "Run in UDP client mode. Send mode: %d, Bind to %s:%d, Dst: %s:%d. IPD: %f sec, Payload Size: %d byte",
            CLIENT_MODE,
            SRC_ADDR[0],
            SRC_ADDR[1],
            SRV_ADDR[0],
            SRV_ADDR[1],
            IPD,
            PAYLOAD_SIZE,
        )
        run_client()
