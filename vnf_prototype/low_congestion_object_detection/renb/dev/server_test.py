#!/usr/bin/python

from subprocess import check_output
import socket
import fcntl
import struct
import cv2
import time
import numpy as np
from yolo import detector
import server_funcs as sf

MSGLEN = 432 * 320 * 3


def bytes_to_img(bytes_str):
    img_arr = np.asarray(bytearray(bytes_str), dtype=np.uint8)
    img = cv2.imdecode(img_arr, cv2.IMREAD_COLOR)
    return img


def get_ip_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa(fcntl.ioctl(
        s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', ifname[:15])
    )[20:24])


def receive_from():
    # collecting infomation of objects
    obj_info = sf.ObjInfo()
    yolo = obj_info.yolo
    # initialize socket
    address = ('10.0.0.252', 8888)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(address)
    s.listen(5)
    conn, addr = s.accept()
    print("client connected")
    data = b''
    cnt = 0
    start = time.time()
    while True:
        while len(data) < 4:
            data += conn.recv(4096)
        info = data[:4]
        data = data[4:]
        msg_len = struct.unpack('>L', info)[0]
        while len(data) < msg_len:
            data = data + conn.recv(4096)
        print(msg_len, len(data))
        frame_data = data[:msg_len]
        data = data[msg_len:]
        try:
            img = bytes_to_img(frame_data)
            cnt += 1
            # current timestamp when receive a complete image frame
            cur = time.time()
            print("an image is completely received, shape is: {}".format(
                img.shape), "fps: ", cnt / (cur - start))
            # add info for calculating velocity
            obj_info.update_info((cur, cnt, yolo.inference(img)))
        except Exception as e:
            print(e, "current image error, abandoned")
    s.close()
    return 0


result = receive_from()
