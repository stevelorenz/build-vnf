#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Server
"""

import argparse
import csv
import json
import os
import socket
import sys
import time

import psutil

import preprocessor
import detector
import log
import rtp


class Server:
    def __init__(
        self,
        topo_params_file,
        no_sfc,
        verbose,
    ):

        with open(topo_params_file, "r") as f:
            topo_params = json.load(f)

        self.server_name = socket.gethostname()
        self.index = int(self.server_name[len("server") :])
        self.client_name = f"client{self.index}"

        self.server_ip = topo_params["servers"].get(self.server_name, None)["ip"][:-3]
        self.client_ip = topo_params["clients"].get(self.client_name, None)["ip"][:-3]

        self.server_address_control = (self.server_ip, topo_params["control_port"])
        self.sock_control = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock_control.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        if no_sfc:
            data_port = "no_sfc_port"
        else:
            data_port = "sfc_port"
        self.client_address_data = (self.client_ip, topo_params[data_port])
        self.server_address_data = (self.server_ip, topo_params[data_port])
        self.sock_data = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        # A buffer for current fragments
        self.fragment_buf = []

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
            self.logger.info(f"Bind data socket to {self.server_address_data}")
            self.logger.info(f"Client data address: {self.client_address_data}")
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
        durations = list()
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
            durations.append(duration)
            # TODO: Measure the computational time of local server inference and store them.
            print(f"[{r}], duration: {duration} s")

    def warm_up(self, det, mode):
        self.logger.info("Warm-up the detector")
        start = time.time()
        raw_img_data = det.read_img_jpeg_bytes("./pedestrain.jpg")
        if mode == "raw":
            # Warm up the session, first time inference is slow
            ret = det.inference(raw_img_data)
            ret = det.get_detection_results(*ret)
        elif mode == "preprocessed":
            prep = preprocessor.Preprocessor()
            compressed_img_data = prep.inference(raw_img_data, 70)
            ret = det.inference(compressed_img_data)
            ret = det.get_detection_results(*ret)
        duration = time.time() - start
        self.logger.info(f"Warm-up the mode {mode} finished! Takes {duration} seconds")

    @staticmethod
    def sort_fragments(fragments):
        """Sort the given fragments in-place with fragment_offset
        It's unexpected that RTP fragments come out-of-order from DPDK VNF...
        It may be a new "research question"...

        :param fragments:
        """
        fragments.sort(key=lambda f: f.fragment_offset)

    def recv_frame(self, num_fragments=-1):
        """It's naive... It assumes no packet losses yet..."""
        marker = 0
        fragments = []
        reassembler = rtp.RTPReassembler()
        while marker != 1:
            data, _ = self.sock_data.recvfrom(1500)
            fragment = rtp.RTPJPEGPacket.unpack(data)
            fragments.append(fragment)
            marker = fragment.marker

        if num_fragments > 0:
            if len(fragments) != num_fragments:
                self.logger.error(
                    f"Fragment number is wrong. Expect: {num_fragments}, actual: {len(fragments)}"
                )

        self.sort_fragments(fragments)

        for fragment in fragments[:-1]:
            reassembler.add_fragment(fragment)
        ret = reassembler.add_fragment(fragments[-1])

        assert ret == "HAS_ENTIRE_FRAME"
        return reassembler.get_frame()

    def send_resp(self, resp):
        data = (json.dumps(resp)).encode("ascii")
        self.sock_data.sendto(data, self.client_address_data)

    def run_store_forward(self, num_frames):
        det = detector.Detector(mode="raw")
        self.warm_up(det, "raw")

        proc_latencies = []
        for _ in range(num_frames):
            start = time.time()
            frame = self.recv_frame()
            ret = det.inference(frame)
            resp = det.get_detection_results(*ret)
            self.send_resp(resp)
            duration = time.time() - start
            proc_latencies.append(duration)
        self.logger.info("All frames are received! Stop the server!")
        return proc_latencies

    def run_compute_forward(self, num_frames):
        det = detector.Detector(mode="preprocessed")
        self.warm_up(det, "preprocessed")

        proc_latencies = []
        for _ in range(num_frames):
            start = time.time()
            frame = self.recv_frame(97)
            ret = det.inference(frame)
            resp = det.get_detection_results(*ret)
            self.send_resp(resp)
            duration = time.time() - start
            proc_latencies.append(duration)
        self.logger.info("All frames are received! Stop the server!")
        return proc_latencies

    def run(self, mode, num_frames, result_dir, fake_inference):
        """MARK: Packet losses are not considered yet!"""
        self.logger.info(
            f"Run server main loop with mode {mode}, number of frames: {num_frames}"
        )
        if mode == "server_local":
            self.run_server_local(30, fake_inference)
            proc_latencies = []
        elif mode == "store_forward":
            proc_latencies = self.run_store_forward(num_frames)
        elif mode == "compute_forward":
            proc_latencies = self.run_compute_forward(num_frames)

        p = os.path.join(result_dir, f"server_process_latency_{mode}.csv")
        with open(p, "a+") as csvfile:
            writer = csv.writer(csvfile, delimiter=",", lineterminator="\n")
            writer.writerow(proc_latencies)

    def cleanup(self):
        self.sock_control.close()
        self.sock_data.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Server")
    parser.add_argument(
        "-n",
        "--num_frames",
        type=int,
        default=3,
        help="Total number of frames to send",
    )
    parser.add_argument(
        "-m",
        "--mode",
        type=str,
        default="store_forward",
        choices=["store_forward", "compute_forward", "server_local"],
        help="Working mode of the server",
    )
    parser.add_argument(
        "--topo_params_file",
        type=str,
        default="./share/dumbbell.json",
        help="Path of the dumbbell topology JSON file",
    )
    parser.add_argument(
        "--result_dir",
        type=str,
        default="./share/result",
        help="Directory to store measurement results",
    )
    parser.add_argument(
        "--no_sfc", action="store_true", help="Use No SFC port, just for test"
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

    server = None
    try:
        server = Server(
            args.topo_params_file,
            args.no_sfc,
            args.verbose,
        )
        server.setup(args.mode)
        server.run(args.mode, args.num_frames, args.result_dir, args.fake_inference)
    except KeyboardInterrupt:
        print("KeyboardInterrupt! Stop server!")
    except RuntimeError as e:
        print("Server stops with error: " + str(e))
    finally:
        if server:
            server.cleanup()
