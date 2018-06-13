#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8


"""
About: Forwarding UDP segments with XDP
Email: xianglinks@gmail.com
"""

import ctypes as ct
import sys
import time

from bcc import BPF

import pyroute2
