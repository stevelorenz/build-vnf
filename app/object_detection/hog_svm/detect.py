#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Simple pedestrian detection with OpenCV

       Just want to have a view of steps and methods involved in the pedestrian
       detection and try to find something that can be offloaded to the VNFs

Ref  : https://www.pyimagesearch.com/2015/11/09/pedestrian-detection-opencv/
"""

import time

import cv2

import imutils
import numpy as np
from imutils.object_detection import non_max_suppression

if __name__ == "__main__":
    # Init HOG descriptor
    hog = cv2.HOGDescriptor()
    # Set SVM to be pre-trained pedestrian detector
    hog.setSVMDetector(cv2.HOGDescriptor_getDefaultPeopleDetector())

    # Apply detector to the image
    image = cv2.imread("../../dataset/image/person_and_bike_006.bmp")
    image = imutils.resize(image, width=min(400, image.shape[1]))
    orig = image.copy()

    # Detect people in the image
    # MARK: This step takes already several hundreds millisecond.
    #       Check which steps can be offloaded into network functions
    start = time.time()
    (rects, weights) = hog.detectMultiScale(
        image, winStride=(4, 4), padding=(8, 8), scale=1.05
    )
    print("# Time for HOG detection: {}ms".format((time.time() - start) * 1000.0))

    # Draw the original bounding boxes
    for (x, y, w, h) in rects:
        cv2.rectangle(orig, (x, y), (x + w, y + h), (0, 0, 255), 2)

    # Apply non-maxima suppression to the bounding boxes
    rects = np.array([[x, y, x + w, y + h] for (x, y, w, h) in rects])
    pick = non_max_suppression(rects, probs=None, overlapThresh=0.65)

    # draw the final bounding boxes
    for (xA, yA, xB, yB) in pick:
        cv2.rectangle(image, (xA, yA), (xB, yB), (0, 255, 0), 2)

    cv2.imshow("Before NMS", orig)
    cv2.imshow("After NMS", image)
    cv2.waitKey(0)
