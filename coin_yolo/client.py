#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Client
"""

import argparse
import csv
import socket
import sys
import time

import log
import rtp_packet


class Client:
    """Client"""

    def __init__(
        self,
        server_address_control,
        client_address_data,
        server_address_data,
        verbose,
    ):
        self.server_address_control = server_address_control
        self.sock_control = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_address_data = client_address_data
        self.server_address_data = server_address_data
        self.sock_data = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock_data.bind(self.client_address_data)

        if verbose:
            self.logger = log.get_logger("debug")
        else:
            self.logger = log.get_logger("info")

    def setup(self):
        self.logger.info("Setup client")

    # def start_session(self, wav_range, source_number, total_msg_num):
    #     """Start a new session for messages with specific parameters."""
    #     self.sock_control.connect(self.server_address_control)
    #     context = ",".join((str(wav_range), str(source_number), str(total_msg_num)))
    #     self.sock_control.sendall(context.encode())
    #     ack = self.sock_control.recv(1024).decode()
    #     if ack != "INIT_ACK":
    #         raise RuntimeError("Failed to get the INIT ACK from server!")
    #     self.sock_control.sendall("CONTROL_CLOSE".encode())

    def warmup(self):
        pass

    def run(self, mode: str, mps: int):
        pass
        # self.start_session(wav_range, source_number, total_msg_num)

    def cleanup(self):
        self.sock_control.close()
        self.sock_data.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Client")
    parser.add_argument(
        "-m",
        "--mode",
        type=str,
        default="store_forward",
        choices=["store_forward", "compute_forward"],
        help="The working mode of the client",
    )
    parser.add_argument(
        "-d", "--duration", type=int, default=60, help="Duration of one iteration"
    )
    parser.add_argument(
        "--mps",
        type=int,
        default=int(1e5),
        help="Messages-per-second, the same as mps in sockperf",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Make the client more talkative"
    )
    args = parser.parse_args()

    client_address_data = ("10.0.1.11", 9999)
    server_address_data = ("10.0.3.11", 9999)
    # data port + 1
    server_address_control = (server_address_data[0], server_address_data[1] + 1)

    try:
        client = Client(
            server_address_control,
            client_address_data,
            server_address_data,
            args.verbose,
        )
        client.setup()
    except KeyboardInterrupt:
        print("KeyboardInterrupt! Stop client!")
    finally:
        client.cleanup()
