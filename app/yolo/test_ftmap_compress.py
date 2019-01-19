#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Try to compress the feature maps (output of middle layers of the YOLOv2's
        convolutional neutral network (CNN))
Email: xianglinks@gmail.com
"""

import os
import cv2
import numpy as np
from matplotlib import pyplot as plt


def get_feature_map(feature_map, is_display=0, is_save=0):
    """Get and plot feature map with normalized pixels from feature_map ndarray

    :param feature_map (numpy.ndarray): Shape: (width, height, channel)
    """
    m, n, c = feature_map.shape
    a = int(np.sqrt(c))
    while c % a is not 0:
        a = a - 1
    b = int(c / a)
    imgs = []
    cur = []

    for i in range(0, a):
        for j in range(0, b):
            tmp = np.zeros(shape=(feature_map.shape[0], feature_map.shape[1]))

            f_tmp = cv2.normalize(
                feature_map[:, :, i * j], tmp, 0, 255, cv2.NORM_MINMAX, cv2.CV_8UC1)

            cur.append(f_tmp)
        f_tmp = np.hstack(cur)
        imgs.append(f_tmp)
        cur = []
    res = np.vstack(imgs)
    if is_display:
        cv2.imshow("feature", res)
        cv2.waitKey(0)
    if is_save:
        cv2.imwrite("./feature.jpg", res)
    return feature_map


def main():
    """main"""
    fm_shape = (76, 76, 128)
    # Load the binary output of the darknet into a numpy array
    raw_data = np.fromfile("./29_output.bin", dtype=np.float32)
    # Iterate over 128 channels
    fm_arr = np.zeros(fm_shape, dtype=np.float32)
    step = fm_shape[0] * fm_shape[1]
    for i in range(128):
        fm_arr[:, :, i] = raw_data[
            i * step:(i+1) * step
        ].reshape(fm_shape[:2])
    # TODO: Use DCT or wavelet to make the feature maps sparse then use random
    # matrix for sampling (compressed sensing)
    get_feature_map(fm_arr, 1)

    # TODO: Try different compress methods

    raw_data.tofile("./29_output_compd.bin")


if __name__ == '__main__':
    main()
