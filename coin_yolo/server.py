#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Server
"""

import argparse
import csv
import socket
import sys
import time

import log
import rtp_packet


class Server:
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
        self.sock_data.bind(self.server_address_data)

        if verbose:
            self.logger = log.get_logger("debug")
        else:
            self.logger = log.get_logger("info")

    def cleanup(self):
        self.sock_control.close()
        self.sock_data.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Server")
    parser.add_argument(
        "-m",
        "--mode",
        type=str,
        default="store_forward",
        choices=["store_forward", "compute_forward"],
        help="Working mode of the server",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Make the server more talkative"
    )
    args = parser.parse_args()

    client_address_data = ("10.0.1.11", 9999)
    server_address_data = ("10.0.3.11", 9999)
    server_address_control = (server_address_data[0], server_address_data[1] + 1)

    try:
        server = Server(
            server_address_control,
            client_address_data,
            server_address_data,
            args.verbose,
        )
    except KeyboardInterrupt:
        print("KeyboardInterrupt! Stop server!")
    except RuntimeError as e:
        print("Server stops with error: " + str(e))
    finally:
        server.cleanup()
