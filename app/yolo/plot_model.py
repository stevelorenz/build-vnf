#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import re
import sys
import copy
import numpy as np
sys.path.append("../../evaluation/scripts/")


"""
About: Plot model infos and delay results

Email: xianglinks@gmail.com
"""

NUM_LAYER_MAP = {
    0: "conv",
    3: "pool",
    8: "route",
    22: "region",
    25: "reorg",
}

MODELS = ("yolov2-tiny", "yolov2")
BAR_WIDTH = 0.6


class LABEL_GEN(object):
    """Just make it simple"""

    def __init__(self):
        self.counter = dict.fromkeys(NUM_LAYER_MAP.values(), 0)

    def get_label(self, key):
        label = NUM_LAYER_MAP.get(int(key), None)
        if not label:
            raise RuntimeError("Unknown key value for the layer index map...")
        self.counter[label] += 1
        return str(label) + " " + str(self.counter[label])


def plot_io_sizes(m, just_return=False):
    import tex
    tex.setup(
        width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
        params={
            'hatch.linewidth': 0.5
        }
    )
    # format: (idx, name, input, output)
    label_gen = LABEL_GEN()
    data = list()
    with open("./net_info_%s.csv" % m, "r") as f:
        net_infos = f.readlines()
        for info in net_infos:
            fields = list(map(str.strip, info.split(",")))
            fields = list(map(int, info.split(",")))
            data.append((fields[0], label_gen.get_label(fields[1]), fields[2],
                         fields[3]))

        x = [i[0] for i in data]
        x_labels = [i[1] for i in data]
        in_s = [i[2] for i in data]
        out_s = [i[3] for i in data]

    if just_return:
        return [x, x_labels, out_s]
    else:
        # Plot io_sizes in separate figures
        fig, ax = plt.subplots()
        ax.bar(x, out_s, BAR_WIDTH, color='blue', lw=0.6,
               alpha=0.8, edgecolor='black', label="Output Size")

        # TODO: Improve the looking
        ax.set_xticks(x)
        ax.set_title("%s" % m)
        ax.set_xticklabels(x_labels, rotation="vertical")
        ax.set_ylim(0, 1.2 * 10 ** 7)
        ax.set_ylabel("Output Size (Bytes)")
        tex.save_fig(fig, "./net_info_%s" % m, fmt="png")
        plt.close(fig)


def plot_layer_delay(m, just_return=False):
    """TODO: Plot pdf instead of average"""
    import tex
    tex.setup(
        width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
        params={
            'hatch.linewidth': 0.5
        }
    )
    label_gen = LABEL_GEN()
    data = list()
    with open("./per_layer_delay_%s.csv" % m, "r") as f:
        lines = f.readlines()
        for l in lines:
            fields = list(map(str.strip, l.split(",")))
            fields = list(map(float, fields))
            data.append((int(fields[1]), fields[2], fields[3], fields[0]))

    # Avoid duplicated label
    idx_layer_map = dict()
    layer_len = len(set([int(x[0]) for x in data]))
    x_labels = list()
    for idx in range(layer_len):
        label = label_gen.get_label(data[idx][1])
        idx_layer_map[idx] = label
        x_labels.append(label)

    delay = dict.fromkeys(x_labels)
    tl = list()
    for k in delay.keys():
        delay[k] = copy.deepcopy(tl)
    # Calculate delay
    for d in data:
        delay[idx_layer_map[d[0]]].append(d[2])

    delay_avg = list()
    delay_err = list()
    for l in x_labels:
        delay_avg.append(np.average(delay[l]))
        delay_err.append(np.std(delay[l]))

    x = list(set([int(i[0]) for i in data]))

    if just_return:
        return [x, delay_avg, delay_err]
    else:
        fig, ax = plt.subplots()

        ax.bar(x, delay_avg, BAR_WIDTH, color='blue', lw=0.6,
               yerr=delay_err, ecolor='red', error_kw={"elinewidth": 1},
               alpha=0.8, edgecolor='black', label="Latency")

        ax.set_xticks(x)
        ax.set_xticklabels(x_labels, rotation="vertical")
        ax.set_ylabel("Inference Latency (ms)")
        ax.set_ylim(0)
        tex.save_fig(fig, "./per_layer_delay_%s" % m, fmt="png")
        plt.close(fig)


def plot_shared_x():
    """Maybe look nicer"""
    import tex
    tex.setup(
        width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
        params={
            'hatch.linewidth': 0.5
        }
    )
    for m in MODELS:
        fig, ax_arr = plt.subplots(2, sharex=True)

        ax_arr[0].set_ylabel("Output Size (Bytes)", fontsize=5)
        ax_arr[1].set_ylabel("Inference Latency (ms)", fontsize=5)

        x, x_labels, out_s = plot_io_sizes(m, just_return=True)
        ax_arr[0].bar(x, out_s, BAR_WIDTH, color='blue', lw=0.6,
                      alpha=0.8, edgecolor='black', label="Output Size")
        x, delay_avg, delay_err = plot_layer_delay(m, just_return=True)
        ax_arr[1].bar(x, delay_avg, BAR_WIDTH, color='blue', lw=0.6,
                      yerr=delay_err, ecolor='red', error_kw={"elinewidth": 1},
                      alpha=0.8, edgecolor='black', label="Latency")

        ax_arr[1].set_xticks(x)
        ax_arr[1].set_xticklabels(x_labels, rotation="vertical")
        fig.align_ylabels(ax_arr)
        tex.save_fig(fig, "./combined_%s" % m, fmt="png")


# for m in MODELS:
    # plot_io_sizes(m)
    # plot_layer_delay(m)
plot_shared_x()
