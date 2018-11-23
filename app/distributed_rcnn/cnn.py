#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Test CNN feature extractor and classifier
       This is one component of the original R-CNN algorithm instead of its
       faster variants.  This component SHOULD run on the edge server and not be
       offloaded on the VNFs.

MARK : Use PyTorch for the CNN implementation
"""

import torch
