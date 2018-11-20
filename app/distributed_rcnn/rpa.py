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

import cv2


def test_selective_search():
    print("* Test selective search...")
    pass


def test_edgeboxes():
    print("* Test edge boxes...")
    print("WARN: This needs opencv contrib modules")


if __name__ == "__main__":
    test_selective_search()
    test_edgeboxes()
