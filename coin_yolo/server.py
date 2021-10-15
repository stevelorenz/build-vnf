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
import json

import psutil

import config
import detector
import log
import rtp


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

        self.sock_control = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock_control.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.client_address_data = client_address_data
        self.server_address_data = server_address_data
        self.sock_data = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        if verbose:
            self.logger = log.get_logger("debug")
        else:
            self.logger = log.get_logger("info")

    def setup(self, mode):
        self.logger.info(f"Setup server with mode {mode}")
        if mode == "server_local":
            return
        else:
            self.sock_control.bind(self.server_address_control)
            self.sock_control.listen(1)

            self.sock_data.bind(self.server_address_data)

    @staticmethod
    def fake_inference(duration):
        """Fake it until you make it!"""
        start = time.time()
        i = 0
        while True:
            i = i * i
            d = time.time() - start
            if d >= duration:
                break

    def run_server_local(self, num_rounds, fake_inference):
        self.logger.info(f"Run server local test for {num_rounds} rounds")
        det = detector.Detector(mode="raw")
        data = det.read_img_jpeg_bytes("./pedestrain.jpg")
        # Warm up the session, first time inference is slow
        ret = det.inference(data)
        ret = det.get_detection_results(*ret)
        mem_gb = (psutil.Process().memory_info().rss) / (1024 * 1024 * 1024)
        self.logger.info(f"Tensorflow session is warmed up, memory usage: {mem_gb} GB")
        print("Warm-up inference result:")
        print(ret)
        for r in range(num_rounds):
            start = time.time()
            if fake_inference:
                self.fake_inference(0.8)
            else:
                ret = det.inference(data)
                _ = det.get_detection_results(*ret)
            duration = time.time() - start
            # TODO: Measure the computational time of local server inference and store them.
            print(f"[{r}], duration: {duration} s")

    def run(self, mode, fake_inference):
        """MARK: Packet losses are not considered yet!"""
        if mode == "server_local":
            self.run_server_local(30, fake_inference)
        else:
            self.logger.info("Run server main loop")
            # Just for single connection
            conn, addr = self.sock_control.accept()
            self.logger.info("Get connection from {}:{}".format(*addr))
            while True:
                packet, _ = self.sock_data.recvfrom(4096)
                if len(packet) == 1:
                    if packet.decode() == "H":
                        self.logger.info("Receive Hello packet!")
                        continue
                    elif packet.decode() == "B":
                        break
                else:
                    print("RTP")
            self.logger.info("Receive Byte packet! Server stops")

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
        choices=["store_forward", "compute_forward", "server_local"],
        help="Working mode of the server",
    )
    parser.add_argument(
        "--fake_inference",
        action="store_true",
        help="Use fake inference, assume that a magic GPU is used",
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Make the server more talkative"
    )
    args = parser.parse_args()

    try:
        server = Server(
            config.server_address_control,
            config.client_address_data,
            config.server_address_data,
            args.verbose,
        )
        server.setup(args.mode)
        server.run(args.mode, args.fake_inference)
    except KeyboardInterrupt:
        print("KeyboardInterrupt! Stop server!")
    except RuntimeError as e:
        print("Server stops with error: " + str(e))
    finally:
        server.cleanup()
