#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: Plot some model infos
Email: xianglinks@gmail.com
"""

import re

import matplotlib.pyplot as plt
import numpy as np


def plot_outputsizes():
    pass


# MD: name, cur_idx, output_size
entries = list()
output_sizes = list()

with open("./stderr_log.log") as f:
    err_log = f.readlines()

for l in err_log:
    if l.startswith(" "):
        l = l[l.find("->"):]
        l = l.split("   ")
        entries.append(l)

for l in entries:
    output = l
    print(output)
