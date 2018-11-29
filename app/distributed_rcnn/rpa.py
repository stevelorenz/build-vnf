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

Limitation :



#designing-for-speed
Ref   : https://docs.micropython.org/en/latest/reference/speed_python.html

Email : xianglinks@gmail.com
"""

import logging
import multiprocessing
import os
import random
import socket
import struct
import sys
import time
from pathlib import Path

import cv2 as cv
import numpy as np

# WARN: This is the logger for the main process, DO NOT use it in the child
# processes
logger = logging.getLogger(__name__)


class RPA_EB(object):

    """RPA using Edge Boxes for bounding boxes detection and image compression

    - Metadata:
        Matadata are proposals extracted from the frame.

        - Format (TBD):
        1. [x, y, w, h] dtype=uint8, 4bytes: Axises of frame after compression.
        2. [x, y, w, h] * num_of_bboxes dtype=uint8, 4*num_of_bboxes bytes:
        Detected bounding boxes.

    MARK: The code is written with the consideration of low latency and memory
    usage, namely avoid too much object creation and copying in Python.
    """

    def __init__(self, model_path, shape, buf_size,
                 max_boxes_num=1,
                 keep_order=False):
        """__init__

        :param model_path (str):
        :param shape (tuple):
        :param buf_size (int):
        """

        if keep_order:
            raise RuntimeError("Keep order is NOT implemented.")

        self._keep_order = keep_order
        self._shape = shape
        self._frame_size = shape[0] * shape[1] * shape[2] * 4
        self._last = None
        self._esti_proc_lat = 0.0001
        self._frame_batch = 1
        self._max_boxes_num = max_boxes_num

        self._test = False  # MARK: to be removed, use a dummy io module
        self._verbose = False
        self._visualize = False

        self._multi_process = True
        self._log_total_proc_lat = False
        self._log_per_frame_lat = False
        self._output_to_file = False

        # Frame and metadata buffer
        # TODO: Use the same buffer for metadata and frame data
        element_size = 4  # Currently assume int32
        self._metadata_size = 4 * element_size * (max_boxes_num + 1)
        self._metadata_buf = bytearray(self._metadata_size)
        self._buf_size = buf_size
        self._buf = bytearray(self._buf_size)
        self._data_len = 0

        # Detectors
        self._edge_detector = cv.ximgproc.createStructuredEdgeDetection(
            model_path)
        self._edge_boxes = cv.ximgproc.createEdgeBoxes()
        self._edge_boxes.setMaxBoxes(max_boxes_num)

    def _merge_bboxes(self, boxes, frame, method="max_edge"):
        """Merge bounding boxes

        Method:
            - max_edge: Simplest method
        """
        metadata_arr = np.frombuffer(self._metadata_buf, dtype=np.int32)
        if method == "max_edge":
            # TODO: To be optimized, working code...
            metadata_arr[0] = boxes[:, 0].min()
            metadata_arr[1] = boxes[:, 1].min()
            metadata_arr[2] = (boxes[:, 0] + boxes[:, 2]
                               ).max() - metadata_arr[0]
            metadata_arr[3] = (boxes[:, 1] + boxes[:, 3]
                               ).max() - metadata_arr[1]

            if self._output_to_file:
                cv.rectangle(frame, (metadata_arr[0], metadata_arr[1]),
                             (metadata_arr[0] + metadata_arr[2],
                              metadata_arr[1] + metadata_arr[3]),
                             (0, 0, 255), 2, cv.LINE_AA)
        else:
            raise RuntimeError("Unknown BBox merge method!")

        if self._verbose:
            print(metadata_arr)
            # TODO: Calculate the compression ratio = Uncompressed / Compressed

    def _proc_frame(self, frame_size):
        """Processing the frame with a newly forked process"""
        frame = np.frombuffer(self._buf, count=frame_size, dtype=np.uint8)
        frame = cv.imdecode(frame, cv.IMREAD_UNCHANGED)
        frame.reshape(self._shape)
        if self._shape[-1] == 1:
            frame = cv.cvtColor(frame, cv.COLOR_GRAY2RGB)
        elif self._shape[-1] == 3:
            frame = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
        # Get bounding boxes
        edges = self._edge_detector.detectEdges(
            np.float32(frame) * (1.0 / 255.0))
        orimap = self._edge_detector.computeOrientation(edges)
        edges = self._edge_detector.edgesNms(edges, orimap)
        boxes = self._edge_boxes.getBoundingBoxes(edges, orimap)

        if self._output_to_file:
            frame = cv.cvtColor(frame, cv.COLOR_RGB2BGR)
            cv.imwrite("./{}_a.jpg".format(self._img_ctr), frame)

        # Compress the image based on the bounding boxes
        self._merge_bboxes(boxes, frame)

        if self._output_to_file:
            for box in boxes:
                x, y, w, h = box
                cv.rectangle(frame, (x, y), (x+w, y+h),
                             (0, 255, 0), 1, cv.LINE_AA)
            cv.imwrite("./{}_b.jpg".format(self._img_ctr), frame)
            self._img_ctr += 1

        if self._keep_order:
            # Wait until the last process exists
            if self._last:
                # try:
                #     # ISSUE: waitpid() only wait for the child processes to terminate
                #     # Here requires a child process waiting for another child process.
                #     os.waitpid(self._last, 0)
                # except ChildProcessError as e:
                #     # The last process has already existed
                #     print(e)
                #     pass
                pass

        # Output frames
        if self._multi_process:
            os._exit(0)

    def _run_test(self, frame_num):
        """Run test with images stored in the local path"""
        print("Run in test mode. Image data is read from {}".format(TEST_IMAGE_DIR))
        image_lst = list(map(str, list(Path(TEST_IMAGE_DIR).glob("*.jpg"))))
        image_lst.sort()
        for p in image_lst[:frame_num]:
            with open(p, "rb") as data_in:
                frame_size = data_in.readinto(self._buf)

            if self._multi_process:
                while True:
                    try:
                        pid = os.fork()
                    except OSError:
                        # Can not allocate more memory
                        time.sleep(self._esti_proc_lat)
                        continue
                    else:
                        break

                if pid < 0:
                    raise RuntimeError("Can not fork new processes!")
                if pid == 0:
                    # child processes
                    self._proc_frame(frame_size)
                else:
                    logger.debug("Fork a new child with pid: %d", pid)
                    self._last = pid
                    continue
            else:
                self._proc_frame(frame_size)

        # Avoid zombie processes
        if self._multi_process:
            logger.debug("The main process waits until all children terminate.")
            for _ in range(frame_num):
                os.waitpid(0, 0)

    def _run_usock(self):
        """Mark"""
        pass

    def run(self, frame_batch=3, multi_process=True, log_total_proc_lat=False,
            test=False, frame_num=0, output_to_file=False, verbose=False):

        self._frame_batch = frame_batch
        self._multi_process = multi_process
        self._log_total_proc_lat = log_total_proc_lat
        self._output_to_file = output_to_file
        self._verbose = verbose

        if test:
            self._img_ctr = 0
            self._test = test
            self._run_test(frame_num)
        else:
            pass


################
#  Perf Codes  #
################

MODEL_PATH = "../../model/edge_boxes/model.yml.gz"
TEST_IMAGE_DIR = "../../dataset/pedestrian_walking/"
SHAPE = (432, 320, 3)
BUF_SIZE = 50000
FRAME_NUM = 30
MAX_BOXES_NUM = 5


def set_logger_debug():
    fmt_str = '%(asctime)s %(levelname)-6s %(processName)s %(message)s'
    logging.basicConfig(level=logging.DEBUG,
                        handlers=[logging.StreamHandler()],
                        format=fmt_str)


def just_debug():
    import ipdb
    rpa_eb = RPA_EB(MODEL_PATH, SHAPE, BUF_SIZE, max_boxes_num=MAX_BOXES_NUM)
    rpa_eb.run(test=True, multi_process=False, frame_num=100,
               output_to_file=True, verbose=True)


def perf_mem():
    """Perf the memory usage of the Python program with memory_profiler"""
    from memory_profiler import profile
    set_logger_debug()

    @profile
    def test_multi_proc():
        rpa_eb = RPA_EB(MODEL_PATH, SHAPE, BUF_SIZE)
        rpa_eb.run(test=True)

    @profile
    def test_single_proc():
        rpa_eb = RPA_EB(MODEL_PATH, SHAPE, BUF_SIZE)
        rpa_eb.run(test=True, multi_process=False)

    print("* Run with single process")
    test_single_proc()
    print("* Run with multiple processes")
    test_multi_proc()


def perf_latency():
    """Perf the processing latency"""
    # set_logger_debug()
    print("* Run with single process")
    rpa_eb = RPA_EB(MODEL_PATH, SHAPE, BUF_SIZE)
    st = time.time()
    rpa_eb.run(test=True, multi_process=False, frame_num=FRAME_NUM)
    dur = time.time() - st
    print("** Latency per frame of single process:{}".format(
        dur / FRAME_NUM
    ))

    print("* Run with multiple process")
    rpa_eb = RPA_EB(MODEL_PATH, SHAPE, BUF_SIZE)
    st = time.time()
    rpa_eb.run(test=True, multi_process=True, frame_num=FRAME_NUM)
    dur = time.time() - st
    print("** Latency of multiple processes:{}".format(
        dur / FRAME_NUM
    ))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Error, specify the option!")
    else:
        if sys.argv[1] == '-m':
            print("Run in memory perf mode")
            perf_mem()
        elif sys.argv[1] == '-l':
            print("Run in latency perf mode")
            perf_latency()
        elif sys.argv[1] == '-d':
            print("Run in debug mode")
            just_debug()
