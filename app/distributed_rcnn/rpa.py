#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Region-Proposal Algorithm (RPA)

       RPA of the R-CNN object detection approach. Plane to use this step as a
       preprocessing process that can be offloaded on the VNFs. Based on this
       preprocessing, uninteresting data SHOULD be compressed by the VNF to
       reduce the bandwidth required to transmit images to the destination
       application server (To be tested).

       In order to make it compatible on VNF, the preprocessing SHOULD introduce
       low latency and require NOT too much CPU and MEMORY (In order to make the
       VNF process packets with line-rate, it has normally very limited but
       super fast memory) resources. Due to this consideration, structured edge
       detection and edge boxes bounding boxes proposal are used here. These two
       processes can be paralleled with a worker-producer model.

       This process SHOULD be integrated with CALVIN, so the data IO part with
       DPDK should be integrated. Unix-socket is planed to be used for the data
       communication between DPDK l2fwd and OpenCV RPA. Batch processing MAY be
       added to avoid too much communication and context switch overhead.

Ref   : https://docs.micropython.org/en/latest/reference/speed_python.html#designing-for-speed

Email : xianglinks@gmail.com
"""

import logging
import multiprocessing
import random
import os
import socket
import sys
import time
from pathlib import Path

import cv2 as cv
import numpy as np

logger = logging.getLogger(__name__)

MODEL_PATH = "../../model/edge_boxes/model.yml.gz"
TEST_IMAGE_DIR = "../../dataset/pedestrian_car/"
IMAGE_NUM = 100


class RPA_EB(object):

    """RPA using Edge Boxes for bounding boxes detection and image compression

    MARK: The code is written with the consideration of low latency and memory
    usage, namely avoid too much object creation and copying in Python.
    """

    def __init__(self, model_path, shape=(1267, 387, 3),
                 buf_size=300 * 1024, max_boxes_num=1000, test=False):

        self._test = test
        self._shape = shape
        self._last = None

        # Frame buffer
        self._buf = bytearray(buf_size)
        self._buf_size = buf_size
        self._data_len = 0

        # Detectors
        self._edge_detector = cv.ximgproc.createStructuredEdgeDetection(
            model_path)
        self._edge_boxes = cv.ximgproc.createEdgeBoxes()
        self._edge_boxes.setMaxBoxes(max_boxes_num)

    def _proc_frame(self):
        """Processing the frame with a newly forked process"""
        frame = np.frombuffer(self._buf, np.uint8)
        # frame.reshape(self._shape)

        # Wait until the last exists
        if self._last:
            try:
                # ISSUE: waitpid() only wait for the child processes to terminate
                # Here requires a child process waiting for another child process.
                os.waitpid(self._last, 0)
            except ChildProcessError as e:
                # The last process has already existed
                print(e)
                pass
        else:
            logger.debug("This is the first forked process")

        # Output frames
        os._exit(0)

    def _run_test(self):
        """Run test with images stored in the local path"""
        print("Run in test mode. Image data is read from {}".format(TEST_IMAGE_DIR))
        image_lst = list(map(str, list(Path(TEST_IMAGE_DIR).glob("*.png"))))
        children = list()
        for p in image_lst[:5]:
            with open(p, "rb") as data_in:
                ret = data_in.readinto(self._buf)
            # last = pid
            pid = os.fork()
            if pid < 0:
                raise RuntimeError("Can not fork new processes!")
            if pid == 0:
                # child processes
                self._proc_frame()
                # wait last
            else:
                self._last = pid
                children.append(pid)
                continue

        # Avoid zombie processes
        for pid in children:
            os.waitpid(pid, 0)

    def _run_usock(self):
        pass

    def run(self):
        if self._test:
            self._run_test()
        else:
            pass


################
#  Perf Codes  #
################


def perf_mem():
    """Perf the memory usage of the Python program with memory_profiler"""
    from memory_profiler import profile

    @profile
    def test_run():
        rpa_eb = RPA_EB(MODEL_PATH, test=True)
        rpa_eb.run()

    test_run()


def perf_latency():
    pass


################
#  Test Codes  #
################
# MARK: Just for learning, to be removed latter
# Pre-trained model from OpenCV extra
MODEL_PATH = "../../model/edge_boxes/model.yml.gz"
ORIG_PIC_PATH = "../../dataset/objects_2011_b/labeldata/pos/I1_2009_12_14_drive_0004_000101.png"
# Image data for pedestrian and cars
TEST_IMAGE_DIR = "../../dataset/pedestrian_car/"
IMAGE_NUM = 100
MAX_BOXES_NUM = 17
RESULT_FILE_NAME = "rpa_proc_time"


def test_edgeboxes():
    print("* Test edge boxes...")
    print("WARN: This needs opencv contrib modules")
    print("** Load pre-trained model for edge boxes")
    print("The size of the model: {} MB".format(
        os.path.getsize(MODEL_PATH) / (1024 ** 2)))
    edge_detector = cv.ximgproc.createStructuredEdgeDetection(MODEL_PATH)
    edge_boxes = cv.ximgproc.createEdgeBoxes()
    edge_boxes.setMaxBoxes(MAX_BOXES_NUM)

    image_lst = list(map(str, list(Path(TEST_IMAGE_DIR).glob("*.png"))))
    proc_time_lst = list()
    print("** Run edge boxes on {} images in {}".format(IMAGE_NUM,
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
    np.savetxt(RESULT_FILE_NAME + "_{}".format(IMAGE_NUM) +
               ".csv", arr, fmt="%.18e", delimiter=",")


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
        edge_boxes.getBoundingBoxes(edges, orimap)
        slp_t = (1.0 / fps) - (time.time() - st)
        time.sleep(slp_t)
        # TODO: Compress the image for transfer based on proposed edge boxes
        i += 1
        if i == IMAGE_NUM - 1:
            i = 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Error, specify the option!")
    else:
        if sys.argv[1] == '-m':
            print("Run in memory perf mode")
            perf_mem()
