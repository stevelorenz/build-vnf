#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""

"""

import cv2
import numpy as np

import socket
import sys
import os
import time
import struct

if __name__ == "__main__":
    server_address = '/uds_socket'
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        sock.connect(server_address)
    except socket.error as msg:
        print(msg)
        sys.exit(1)
    imgs_path = '/app/yolov2/cocoapi/images/val2017'
    img_names = sorted(os.listdir(imgs_path))
    img_paths = [os.path.join(imgs_path, img_name) for img_name in img_names]
    f = open(img_paths[0], 'rb')
    data = f.read()
    for i, img_name in enumerate(img_paths):
        with open(img_name, 'rb') as f:
            data = f.read()
        sock.sendall(struct.pack('>L', len(data)) + data)
        print("image_idx:{}, len: {}".format(i, len(data) + 4))
time.sleep(2)
