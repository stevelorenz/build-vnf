#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
#

"""
About: Try to perform the RPA stage of the R-CNN
       i) Selective Search
       ii) EdgeBoxes
       iii) Objectness

MARK : The idea is to split the Region-Proposal Algorithm (RPA) from the object
       detection application and run it on the VNF. Then it is essential to
       evaluate the resource

Ref  : https://github.com/rbgirshick/py-faster-rcnn
"""

import time
from os import path

import cv2 as cv

import numpy as np

# Pre-trained model from OpenCV extra
MODEL_PATH = "../../model/edge_boxes/model.yml.gz"
ORIG_PIC_PATH = "../../dataset/objects_2011_b/labeldata/pos/I1_2009_12_14_drive_0004_000101.png"
# ORIG_PIC_PATH = "../../dataset/image/lena.jpg"


def test_edgeboxes():
    print("* Test edge boxes...")
    print("WARN: This needs opencv contrib modules")

    im = cv.imread(ORIG_PIC_PATH)
    # im = cv.cvtColor(im, cv.COLOR_BGR2RGB)
    cv.imwrite("./test_pic.png", im)
    print("The size of the model: {} MB".format(
        path.getsize(MODEL_PATH) / (1024 ** 2)))
    edge_detector = cv.ximgproc.createStructuredEdgeDetection(MODEL_PATH)
    last = time.time()
    # Structured Forests generate edges and orientation map
    edges = edge_detector.detectEdges(np.float32(im) * (1.0 / 255.0))
    print("Edges map before NMS: ")
    print(edges)
    orimap = edge_detector.computeOrientation(edges)
    # MARK: the results are really sparse ---> lower overhead to the bandwidth
    edges = edge_detector.edgesNms(edges, orimap)
    print("Edges map after NMS: ")
    print(edges)
    print("Orientation map:")
    print(orimap)  # not sparse
    print("Time duration for edge detection and NMS: {}".format(
        time.time() - last))

    last = time.time()
    edge_boxes = cv.ximgproc.createEdgeBoxes()
    edge_boxes.setMaxBoxes(20)
    boxes = edge_boxes.getBoundingBoxes(edges, orimap)
    print("Time duration for createing edge boxes: {} seconds".format(
        time.time() - last))

    for box in boxes:
        x, y, w, h = box
        cv.rectangle(im, (x, y), (x+w, y+h), (0, 255, 0), 1, cv.LINE_AA)

    edges_v = edges * 255.0
    cv.imwrite("./test_pic_edges.png", edges_v)
    cv.imwrite("./test_pic_bouding_boxes.png", im)


if __name__ == "__main__":
    test_edgeboxes()
