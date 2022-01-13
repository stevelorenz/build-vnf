#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: A crazy and selfish client that just shoots RTP packets (CBR) to the
       server and pray for the response from the server for object detection.
"""

import argparse
import csv
import json
import socket
import sys
import time

import cv2

import log
import rtp


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

        if verbose:
            self.logger = log.get_logger("debug")
        else:
            self.logger = log.get_logger("info")

    def setup(self):
        self.logger.info("Setup client")
        self.sock_data.bind(self.client_address_data)

    def send_hello(self):
        for _ in range(3):
            self.sock_data.sendto("H".encode(), self.server_address_data)

    def send_bye(self):
        for _ in range(3):
            self.sock_data.sendto("B".encode(), self.server_address_data)

    def imread_jpeg(self, img_path):
        im = cv2.imread(img_path)
        # The image is always resized and re-coded.
        im = cv2.resize(im, (608, 608))
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 100]
        _, im_enc = cv2.imencode(".jpg", im, encode_param)
        return im_enc.tobytes()

    def get_rtp_packets(self, im_data):
        pass

    def send_frame(self):
        pass

    def run(self, mode: str, num_frames: int):
        """MARK: Should I make the send the recv concurrently?"""
        self.logger.info(f"Run client with mode:{mode}, number of frames: {num_frames}")
        self.sock_control.connect(self.server_address_control)
        self.send_hello()
        self.send_bye()

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
        "-f", "--num_frames", type=int, default=17, help="Number of frames to send"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Make the client more talkative"
    )
    args = parser.parse_args()

    client = None
    try:
        client = Client(
            config.server_address_control,
            config.client_address_data,
            config.server_address_data,
            args.verbose,
        )
        client.setup()
        client.run(args.mode, args.num_frames)
    except KeyboardInterrupt:
        print("KeyboardInterrupt! Stop client!")
    finally:
        if client:
            client.cleanup()
