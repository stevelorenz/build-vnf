#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: YOLOv2 Detector
"""


import json
import os
import time
import timeit
import struct
import gc

import cv2
import psutil
import numpy as np

# Disable printing too much warning messages
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "3"
import tensorflow as tf

import preprocessor
from utils.config import class_names, anchors
from utils.imgutils import decode_result, postprocess, image_to_feature_maps


class Detector(object):
    """YOLOv2 object detector
    Code here really needs refactoring...
    """

    def __init__(self, mode="raw", model_path="./model/yolo.pb"):
        self._name = "yolo"
        self._model_path = model_path
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

        self.sess = self._read_model(self._model_path, self._name, is_onecore=True)
        self._init_tensors(self._input_tensor_name, self._output_tensor_name)

    @staticmethod
    def _read_model(path, name, is_onecore=True):
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

        with tf.gfile.FastGFile(path, "rb") as f:
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

    @staticmethod
    def read_img_jpeg_bytes(img_path):
        img = cv2.imread(img_path)
        img = cv2.resize(img, (608, 608))
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 100]
        _, encimg = cv2.imencode(".jpg", img, encode_param)
        encimg = encimg.tobytes()
        return encimg

    @staticmethod
    def preprocess_image(image, image_size=(608, 608)):
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
        l1 = struct.unpack("<H", data[2:4])[0]
        # lp = struct.unpack(">I", data[4:8])[0]
        header_tmp = np.frombuffer(data[8 : 8 + l1], self.dtype_header)
        payload_tmp = np.frombuffer(data[8 + l1 :], self.dtype_payload)
        # predict
        feature_maps = image_to_feature_maps(
            [(payload_tmp, header_tmp)], self.shape_splice
        )
        res = self.sess.run(self.output1, feed_dict={self.input1: feature_maps})
        bboxes, obj_probs, class_probs = decode_result(
            model_output=res,
            output_sizes=(608 // 32, 608 // 32),
            num_class=len(class_names),
            anchors=anchors,
        )
        # image_shape: original image size for displaying
        bboxes, scores, class_max_index = postprocess(
            bboxes, obj_probs, class_probs, image_shape=(432, 320)
        )

        return bboxes, scores, class_max_index, class_names

    def inference(self, data):
        if self._mode == "preprocessed":
            ret = self.inference_preprocessed(data)
            return ret
        elif self._mode == "raw":
            ret = self.inference_raw(data)
            return ret

    @staticmethod
    def get_detection_results(bboxes, scores, class_max_index, class_names, thr=0.3):
        """get_detection_results"""
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


def test():
    raw_img_data = Detector.read_img_jpeg_bytes("./pedestrain.jpg")
    det = Detector(mode="raw")
    ret = det.inference(raw_img_data)
    resp = det.get_detection_results(*ret)
    print("*** Inference result of raw image!")
    print(resp)
    del det
    gc.collect(1)
    gc.collect(2)

    # Test detection of preprocessed image
    prep = preprocessor.Preprocessor()
    compressed_img_data = prep.inference(raw_img_data, 70)
    print(
        f"*** Raw image size: {len(raw_img_data)}B, preprocessed image size: {len(compressed_img_data)}B"
    )
    det = Detector(mode="preprocessed")
    ret = det.inference(compressed_img_data)
    resp_prep = det.get_detection_results(*ret)
    print("*** Inference result of preprocessed image!")
    print(resp_prep)


def benchmark():
    """benchmark"""
    print("* Run benchmark of detector performance")
    setup = """
from __main__ import Detector
data = Detector.read_img_jpeg_bytes("./pedestrain.jpg")
detector = Detector(mode="raw")
# Warm-up the session
ret = detector.inference(data)
    """
    stmt = """
ret = detector.inference(data)
    """
    number = 17
    ret = timeit.timeit(stmt=stmt, setup=setup, number=number)
    ret = ret / number
    print(f"Inference time for one raw image: {ret} s")

    gc.collect()

    setup = """
from __main__ import Detector, preprocessor
data = Detector.read_img_jpeg_bytes("./pedestrain.jpg")
prep = preprocessor.Preprocessor()
compressed_img_data = prep.inference(data, 70)
detector = Detector(mode="preprocessed")
# Warm-up the session
ret = detector.inference(compressed_img_data)
    """
    stmt = """
ret = detector.inference(compressed_img_data)
    """
    number = 17
    ret = timeit.timeit(stmt=stmt, setup=setup, number=number)
    ret = ret / number
    print(f"Inference time for one preprocessed image: {ret} s")


def main():
    """main"""
    # Use RSS to estimate the memory usage of detector
    # data = Detector.read_img_jpeg_bytes("./pedestrain.jpg")
    # detector = Detector(mode="raw")
    # _ = detector.inference(data)
    # mem = psutil.Process().memory_info().rss / (1024 ** 2)
    # print(f"Memory usage: {mem} MB")

    test()
    # benchmark()


if __name__ == "__main__":
    main()
