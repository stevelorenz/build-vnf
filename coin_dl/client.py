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
import os
import socket
import sys
import threading
import time

import cv2

import log
import rtp


class Client:
    """Client"""

    def __init__(
        self,
        topo_params_file,
        no_sfc,
        verbose,
    ):
        # It looks complex...
        with open(topo_params_file, "r") as f:
            topo_params = json.load(f)

        self.host_name = socket.gethostname()
        self.index = int(self.host_name[len("client") :])
        self.server_name = f"server{self.index}"
        self.client_ip = topo_params["clients"].get(self.host_name, None)["ip"][:-3]
        self.server_ip = topo_params["servers"].get(self.server_name, None)["ip"][:-3]
        # Tuple (HOST, PORT)
        if no_sfc:
            data_port = "no_sfc_port"
        else:
            data_port = "sfc_port"
        self.client_address_data = (self.client_ip, int(topo_params[data_port]))
        self.server_address_control = (self.server_ip, int(topo_params["control_port"]))
        self.server_address_data = (self.server_ip, int(topo_params[data_port]))

        self.sock_control = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock_data = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        if verbose:
            self.logger = log.get_logger("debug")
        else:
            self.logger = log.get_logger("info")

    def setup(self):
        self.logger.info("Setup client")
        self.logger.info(f"Bind data socket to address: {self.client_address_data}\n")
        self.sock_data.bind(self.client_address_data)

    def imread_jpeg(self, img_path):
        im = cv2.imread(img_path)
        # The image is always resized and re-coded.
        im = cv2.resize(im, (608, 608))
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 100]
        _, im_enc = cv2.imencode(".jpg", im, encode_param)
        return im_enc.tobytes()

    def prepare_frames(self, num_frames):
        frames = []
        raw_img_data = self.imread_jpeg("./pedestrain.jpg")
        self.logger.info(f"Size of raw image: {len(raw_img_data)} B")
        fragmenter = rtp.RTPFragmenter()
        # MARK: timestamps are not used...
        for i in range(num_frames):
            fragments = fragmenter.fragmentize(raw_img_data, i, 0, 1400)
            frames.append(fragments)
        self.logger.info(f"Number of fragments in a frame: {len(frames[0])}")
        return frames

    def send_frame(self, frame_index, frame):
        for idx, fragment in enumerate(frame):
            data = fragment.pack()
            self.sock_data.sendto(data, self.server_address_data)
            # The first frame may trigger the deployment of new OpenFlow rules.
            if frame_index == 0 and idx < 2:
                time.sleep(0.3)
            # time.sleep(0.01)

    def recv_resp(self, sequence_number):
        data, _ = self.sock_data.recvfrom(1500)
        resp = json.loads(data.decode("ascii"))
        return resp

    def request_yolo_service(self, frames):
        service_latencies = []
        for idx, frame in enumerate(frames):
            self.logger.info(f"Current frame index: {idx}")
            self.send_frame(idx, frame)
            start = time.time()
            _ = self.recv_resp(idx)
            duration = time.time() - start
            service_latencies.append(duration)
            time.sleep(0.5)
        return service_latencies

    def run(self, num_frames: int, result_dir, result_suffix):
        self.logger.info(f"Run client. Number of frames to send: {num_frames}")
        frames = self.prepare_frames(num_frames)
        assert len(frames) == num_frames
        resp_latencies = self.request_yolo_service(frames)

        with open(
            os.path.join(result_dir, f"client_response_latency{result_suffix}.csv"),
            "a+",
        ) as csvfile:
            writer = csv.writer(csvfile, delimiter=",", lineterminator="\n")
            writer.writerow(resp_latencies)

    def cleanup(self):
        self.sock_control.close()
        self.sock_data.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="RTP client")
    parser.add_argument(
        "-n",
        "--num_frames",
        type=int,
        default=3,
        help="Total number of frames to send",
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
        "--result_suffix",
        type=str,
        default="",
        help="The suffix to append to the result CSV file",
    )
    parser.add_argument(
        "--no_sfc", action="store_true", help="Use No SFC port, just for test"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Make the client more talkative"
    )
    args = parser.parse_args()

    if not os.path.isfile(args.topo_params_file):
        raise RuntimeError(
            f"Can not find the topology JSON file: {args.topo_params_file}"
        )

    if not os.path.isdir(args.result_dir):
        print(
            f"Create result directory to store measurement results: {args.result_dir}"
        )
        os.mkdir(args.result_dir)

    client = None
    try:
        client = Client(
            args.topo_params_file,
            args.no_sfc,
            args.verbose,
        )
        client.setup()
        client.run(args.num_frames, args.result_dir, args.result_suffix)
    except KeyboardInterrupt:
        print("KeyboardInterrupt! Stop client!")
    finally:
        if client:
            client.cleanup()
