#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Test YOLO preprocessing

Author: zrbzrb1106
Original source: https://github.com/zrbzrb1106/yolov2/blob/master/client/test_preprocess.py
"""

import os
import socket
import struct
import sys
import time

import cv2

if __name__ == "__main__":

    server_address = "/uds_socket"
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect(server_address)
    except socket.error as msg:
        print(msg)
        sys.exit(1)

    imgs_path = "./lola_yolo_testset/"
    img_names = sorted(os.listdir(imgs_path))
    img_paths = [os.path.join(imgs_path, img_name) for img_name in img_names]

    for i, img_name in enumerate(img_paths[:3]):
        img = cv2.imread(img_name)
        img = cv2.resize(img, (608, 608))
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 100]
        result, encimg = cv2.imencode(".jpg", img, encode_param)
        encimg = encimg.tobytes()
        sock.sendall(struct.pack(">L", len(encimg)) + encimg)
        print("[TX] Image index:{}, TX size:{} bytes".format(i, len(encimg)))

        time.sleep(2)
        to_read = sock.recv(4)
        to_read = struct.unpack(">L", to_read)[0]
        print("[RX] Length of returned bytes: %d" % to_read)
        _ = sock.recv(to_read)
