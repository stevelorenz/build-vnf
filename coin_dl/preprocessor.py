#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: YOLOv2 preprocessor
"""

import copy
import math
import socket
import struct
import time
import timeit
import os

import psutil
import cv2
import numpy as np

os.environ["TF_CPP_MIN_LOG_LEVEL"] = "3"
import tensorflow as tf
from utils.imgutils import feature_maps_to_image


class CompressorObj(object):
    def __init__(self):
        pass

    def jpeg_enc(self, feature_maps, quality):
        """
        feature_maps: output tensor of feature maps in shape (1, 78, 78, 128)
        quality: quality of JPEG lossy compression from 0 - 100
        return: sliced image of feature maps, pixel from 0 - 255
        """
        shape = (8, 16)
        fmap_images_with_info = feature_maps_to_image(
            feature_maps, shape, is_display=0, is_save=0
        )
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), quality]
        result, encimg = cv2.imencode(".jpg", fmap_images_with_info[0][0], encode_param)
        self.compressed_mem = len(encimg)
        # print(len(encimg), "length of encoded image")
        res = (encimg, fmap_images_with_info[0][1])
        return res

    def webp_enc(self, x, quality):
        shape = (8, 16)
        data = copy.copy(x)
        fmap_images_with_info = feature_maps_to_image(
            data, shape, is_display=0, is_save=0
        )
        encode_param = [int(cv2.IMWRITE_WEBP_QUALITY), quality]
        result, encimg = cv2.imencode(
            ".webp", fmap_images_with_info[0][0], encode_param
        )
        self.compressed_mem = len(encimg)
        print(len(encimg), "length of encoded image")
        res = (encimg, fmap_images_with_info[0][1])
        return res


class Preprocessor(object):
    def __init__(self):
        # tensorflow graph
        self.__model_path = "./model/part1.pb"
        self.__name = "part1"
        self.__input_tensor_name = "input:0"
        self.__output_tensor_name = "Pad_5:0"
        self.sess = self._read_model(self.__model_path, self.__name, is_onecore=False)
        self.input1 = self.sess.graph.get_tensor_by_name(
            "{}/{}".format(self.__name, self.__input_tensor_name)
        )
        self.output1 = self.sess.graph.get_tensor_by_name(
            "{}/{}".format(self.__name, self.__output_tensor_name)
        )
        # compression object
        self.compressor = CompressorObj()
        self.buffer = bytearray(10000000)
        # meta data
        self.shape = (1, 78, 78, 128)
        self.batch_size = self.shape[0]
        self.w = self.shape[1]
        self.h = self.shape[2]
        self.channels = self.shape[3]
        self.dtype_header = np.float16
        self.dtype_payload = np.uint8
        self.payload_length = self.batch_size * 78 * 78 * 128
        self.header_length = self.batch_size * 2 * 2 + 1
        self.header = bytes(self.header_length)
        self.payload = bytes(self.payload_length)
        self.results = bytes(self.header_length + self.payload_length)

    def __setitem__(self, k, v):
        self.k = v

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

    def read_img_jpeg_bytes(self, img_path):
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

    # Code from the student... The variable naming here is bad...
    def inference(self, img_bytes, quality):
        """

        :param img_bytes:
        :param quality:
        """
        data = np.frombuffer(img_bytes, np.uint8)
        img = cv2.imdecode(data, 1)
        img_preprossed = self.preprocess_image(img)
        feature_maps = self.sess.run(
            self.output1, feed_dict={self.input1: img_preprossed}
        )
        # The header manipulation here is bad...
        # A separate Python source for this special payload header should be used...
        h_1 = bytes(self.batch_size)
        h_2 = bytes(0)
        header_tmp = b""
        payload_tmp = b""
        res_bytes = b""
        fmaps_bytes_with_info = self.compressor.jpeg_enc(feature_maps, quality)
        fmaps_data = fmaps_bytes_with_info[0]
        header_tmp += np.array(
            fmaps_bytes_with_info[1], dtype=self.dtype_header
        ).tobytes()
        payload_tmp += np.array(fmaps_data, dtype=self.dtype_payload).tobytes()
        l1 = struct.pack("<H", len(header_tmp))
        lp = struct.pack(">I", len(payload_tmp))
        # res_bytes = h_1 + h_2 + l1 + lp + header_tmp + payload_tmp
        res_bytes = b"".join((h_1, h_2, l1, lp, header_tmp, payload_tmp))

        return res_bytes


def benchmark():
    setup = """
from __main__ import Preprocessor
preprocessor = Preprocessor()
img_bytes = preprocessor.read_img_jpeg_bytes("./pedestrain.jpg")
_ = preprocessor.inference(img_bytes, 70)
    """
    stmt = """
_ = preprocessor.inference(img_bytes, 70)
    """
    number = 17
    ret = timeit.timeit(stmt=stmt, setup=setup, number=number)
    ret = ret / number
    print(f"Time to preprocess one image: {ret}s")


def main():
    preprocessor = Preprocessor()
    img_bytes = preprocessor.read_img_jpeg_bytes("./pedestrain.jpg")
    _ = preprocessor.inference(img_bytes, 70)
    mem = psutil.Process().memory_info().rss / (1024 ** 2)
    print(f"Memory usage: {mem} MB")
    benchmark()


if __name__ == "__main__":
    main()
