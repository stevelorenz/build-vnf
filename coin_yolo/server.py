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

# MARK: Takes a long time to import...
import cv2
import numpy as np
import tensorflow as tf

from utils.config import class_names, anchors
from utils.imgutils import image_to_feature_maps, decode_result, postprocess

import config
import log
import rtp_packet


class Detector(object):
    """YOLOv2 object detector
    Code here needs refactoring...
    """

    def __init__(self, mode="preprocessed"):
        self._name = "yolo"
        self._model_path = "./model/yolo.pb"

        self._mode = mode
        if mode == "preprocessed":
            self._input_tensor_name = "Pad_5:0"
            self._output_tensor_name = "output:0"
        elif mode == "raw":
            self._input_tensor_name = "input:0"
            self._output_tensor_name = "output:0"
        else:
            raise RuntimeError("Invalid server mode!")

        self.dtype_header = np.float16
        self.dtype_payload = np.uint8
        self.shape_splice = (8, 16)

        self.sess = self._read_model(self._model_path, self._name, is_onecore=False)
        self._init_tensors(self._input_tensor_name, self._output_tensor_name)

    def _read_model(self, path, name, is_onecore=True):
        """
        path: the location of pb file path
        name: name of tf graph
        return: tf.Session()
        """
        sess = tf.Session()
        # use one cpu core
        if is_onecore:
            session_conf = tf.ConfigProto(
                intra_op_parallelism_threads=1, inter_op_parallelism_threads=1
            )
            sess = tf.Session(config=session_conf)

        mode = "rb"
        with tf.gfile.FastGFile(path, mode) as f:
            graph_def = tf.GraphDef()
            graph_def.ParseFromString(f.read())
            sess.graph.as_default()
            tf.import_graph_def(graph_def, name=name)
        return sess

    def _init_tensors(self, input_tensor_name, output_tensor_name):
        self.input1 = self.sess.graph.get_tensor_by_name(
            "{}/{}".format(self._name, input_tensor_name)
        )
        self.output1 = self.sess.graph.get_tensor_by_name(
            "{}/{}".format(self._name, output_tensor_name)
        )

    def _read_img_jpeg_bytes(self, img_path):
        img = cv2.imread(img_path)
        img = cv2.resize(img, (608, 608))
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 100]
        _, encimg = cv2.imencode(".jpg", img, encode_param)
        encimg = encimg.tobytes()
        return encimg

    def preprocess_image(self, image, image_size=(608, 608)):
        image_cp = np.copy(image).astype(np.float32)
        # resize image
        image_rgb = cv2.cvtColor(image_cp, cv2.COLOR_BGR2RGB)
        image_resized = cv2.resize(image_rgb, image_size)
        # normalize
        image_normalized = image_resized.astype(np.float32) / 225.0
        # expand dimension
        image_expanded = np.expand_dims(image_normalized, axis=0)
        return image_expanded

    def inference_raw(self, data):
        """Inference the raw image data in bytes"""
        data = np.frombuffer(data, np.uint8)
        img = cv2.imdecode(data, 1)
        img_preprossed = self.preprocess_image(img)
        res = self.sess.run(self.output1, feed_dict={self.input1: img_preprossed})
        bboxes, obj_probs, class_probs = decode_result(
            model_output=res,
            output_sizes=(608 // 32, 608 // 32),
            num_class=len(class_names),
            anchors=anchors,
        )
        bboxes, scores, class_max_index = postprocess(
            bboxes, obj_probs, class_probs, image_shape=(432, 320)
        )

        return bboxes, scores, class_max_index, class_names

    def inference_preprocessed(self, data):
        raise NotImplemented("")

    def inference(self, data):
        """inference"""
        if self._mode == "preprocessed":
            ret = self.inference_preprocessed(data)
            return ret
        elif self._mode == "raw":
            ret = self.inference_raw(data)
            return ret

    def get_detection_results(
        self, bboxes, scores, class_max_index, class_names, thr=0.3
    ):
        results = list()
        for i, box in enumerate(bboxes):
            if scores[i] < thr:
                continue
            cls_indx = class_max_index[i]
            r = {
                "object": class_names[cls_indx],
                "score": float(scores[i]),
                "position": (int(box[0]), int(box[1]), int(box[2]), int(box[3])),
            }
            results.append(r)
        results = json.dumps(results)
        return results


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

    def setup(self):
        self.logger.info("Setup server")
        self.sock_control.bind(self.server_address_control)
        self.sock_control.listen(1)

        self.sock_data.bind(self.server_address_data)

    def imread_jpeg(self, img_path):
        im = cv2.imread(img_path)
        # The image is always resized and re-coded.
        im = cv2.resize(im, (608, 608))
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 100]
        _, im_enc = cv2.imencode(".jpg", im, encode_param)
        return im_enc.tobytes()

    def run_server_local(self, num_rounds):
        self.logger.info(f"Run server local test for {num_rounds} rounds")
        data = self.imread_jpeg("./pedestrain.jpg")
        for r in range(num_rounds):
            start = time.time()
            # Tensorflow caches results by default.
            detector = Detector(mode="raw")
            ret = detector.inference(data)
            ret = detector.get_detection_results(*ret)
            print(ret)
            duration = time.time() - start
            # TODO: Measure the computational time of local server inference and store them.
            print(duration)

    def run(self, mode):
        """MARK: Packet losses are not considered yet!"""
        if mode == "server_local":
            self.run_server_local(10)
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
        server.setup()
        server.run(args.mode)
    except KeyboardInterrupt:
        print("KeyboardInterrupt! Stop server!")
    except RuntimeError as e:
        print("Server stops with error: " + str(e))
    finally:
        server.cleanup()
