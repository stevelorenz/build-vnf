#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#
#

"""
About: Try to perform the RPA stage of the R-CNN
       i) Selective Search
       ii) EdgeBoxes (To be used in this project)
       iii) Objectness

MARK : The idea is to split the Region-Proposal Algorithm (RPA) from the object
       detection application and run it on the VNF. Then it is essential to
       evaluate the required compute resources and the latency performance.
"""

import sys
import time
from os import path
from pathlib import Path

import cv2 as cv

import numpy as np

# Pre-trained model from OpenCV extra
MODEL_PATH = "../../model/edge_boxes/model.yml.gz"
ORIG_PIC_PATH = "../../dataset/objects_2011_b/labeldata/pos/I1_2009_12_14_drive_0004_000101.png"
# Image data for pedestrian and cars
TEST_IMAGE_DIR = "../../dataset/pedestrian_car/"
IMAGE_NUM = 100
MAX_BOXES_NUM = 17
RESULT_FILE = "rpa_proc_time.csv"


def test_edgeboxes():
    print("* Test edge boxes...")
    print("WARN: This needs opencv contrib modules")
    print("** Load pre-trained model for edge boxes")
    print("The size of the model: {} MB".format(
        path.getsize(MODEL_PATH) / (1024 ** 2)))
    edge_detector = cv.ximgproc.createStructuredEdgeDetection(MODEL_PATH)
    edge_boxes = cv.ximgproc.createEdgeBoxes()
    edge_boxes.setMaxBoxes(MAX_BOXES_NUM)

    image_lst = list(map(str, list(Path(TEST_IMAGE_DIR).glob("*.png"))))
    proc_time_lst = list()
    print("** Run edge boxex on {} images in {}".format(IMAGE_NUM,
                                                        TEST_IMAGE_DIR))
    st = time.time()
    for i in range(IMAGE_NUM):
        im = cv.imread(image_lst[i])
        im = cv.cvtColor(im, cv.COLOR_BGR2RGB)
        last = time.time()
        # Structured Forests generate edges and orientation map
        edges = edge_detector.detectEdges(np.float32(im) * (1.0 / 255.0))
        orimap = edge_detector.computeOrientation(edges)
        # MARK: the results are really sparse ---> lower overhead to the bandwidth
        edges = edge_detector.edgesNms(edges, orimap)
        # print("Edges map after NMS: ")
        # print(edges)
        # print("Orientation map:")
        # print(orimap)  # not sparse
        dur_edge = time.time() - last
        # print("Time duration for edge detection and NMS: {}".format(dur_edge))

        last = time.time()
        boxes = edge_boxes.getBoundingBoxes(edges, orimap)
        dur_boxes = time.time() - last
        # print("Time duration for creating edge boxes: {} seconds".format(dur_boxes))

        proc_time_lst.append([dur_edge, dur_boxes])

        # Output the last image to have some visualization
        if i == IMAGE_NUM - 1:
            total_dur = time.time() - st
            print("# The total processing delay for {} images is {} seconds".format(
                IMAGE_NUM, total_dur))
            cv.imwrite("./test_pic.png", im)
            for box in boxes:
                x, y, w, h = box
                cv.rectangle(im, (x, y), (x+w, y+h), (0, 255, 0), 1, cv.LINE_AA)
            # TODO: Compress the image for transfer based on proposed edge boxes
            edges_v = edges * 255.0
            cv.imwrite("./test_pic_edges.png", edges_v)
            cv.imwrite("./test_pic_bouding_boxes.png", im)

    # Save results in CSV
    arr = np.array(proc_time_lst)
    np.savetxt(RESULT_FILE, arr, fmt="%.18e")


def edge_boxes_loop(fps=1):
    """Run edge boxes detection in a infinite loop, this is used for
    computational resource monitoring

    :param fps: Frames per second
    """
    edge_detector = cv.ximgproc.createStructuredEdgeDetection(MODEL_PATH)
    edge_boxes = cv.ximgproc.createEdgeBoxes()
    edge_boxes.setMaxBoxes(MAX_BOXES_NUM)
    image_lst = list(map(str, list(Path(TEST_IMAGE_DIR).glob("*.png"))))

    i = 0
    while True:
        st = time.time()
        im = cv.imread(image_lst[i])
        im = cv.cvtColor(im, cv.COLOR_BGR2RGB)
        edges = edge_detector.detectEdges(np.float32(im) * (1.0 / 255.0))
        orimap = edge_detector.computeOrientation(edges)
        edges = edge_detector.edgesNms(edges, orimap)
        boxes = edge_boxes.getBoundingBoxes(edges, orimap)
        slp_t = (1.0 / fps) - (time.time() - st)
        time.sleep(slp_t)
        # TODO: Compress the image for transfer based on proposed edge boxes
        i += 1
        if i == IMAGE_NUM - 1:
            i = 0


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "-c":
        print("Run general checks")
        print("Use optimized? {}".format(cv.useOptimized()))
    elif len(sys.argv) > 1 and sys.argv[1] == "-l":
        print("Run in infinite loop mode")
        edge_boxes_loop()
    else:
        test_edgeboxes()
