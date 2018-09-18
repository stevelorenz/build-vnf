#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Plot evaluation results
"""

import os
import sys
sys.path.append('./scripts/')
import tex

import numpy as np

import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.pyplot import cm
from scipy import stats

FIG_FMT = 'png'


def calc_hwci(data, confidence=0.95):
    hwci = stats.sem(data) * stats.t._ppf((1+confidence)/2.0, len(data)-1)
    return hwci


def save_fig(fig, path, fmt=FIG_FMT):
    """Save fig to path"""
    fig.savefig(path + '.%s' % fmt, pad_iches=0,
                bbox_inches='tight', dpi=400, format=fmt)


def warn_x_std(value_arr, path=None, ret_inv_lst=False, x=3):
    """Raise exception if the value is not in the x std +- mean value of
    value_arr
    """
    avg = np.average(value_arr)
    std = np.std(value_arr)
    invalid_index = list()

    for idx, value in enumerate(value_arr):
        if (abs(value - avg) >= x * std):
            if not ret_inv_lst:
                if path:
                    error_msg = 'Line: %d, Value: %s is not located in three std range, path: %s' % (
                        (idx + 1), value, path)
                else:
                    error_msg = 'Line: %d, Value: %s is not located in three std range of list: %s' % (
                        (idx + 1), value, ', '.join(map(str, value_arr)))
                raise RuntimeError(error_msg)
            else:
                invalid_index.append(idx)

    print('[DEBUG] Len of value_arr: %d' % len(value_arr))
    return (value_arr, invalid_index)


def label_bar(rects, ax):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2., height + 0.5,
                '%.1f' % height, fontsize=6,
                ha='center', va='bottom')


def plot_bw():
    tex.setup(width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
              params={
                  'hatch.linewidth': 0.5
              }
              )
    PAYLOAD_SIZE = [256, 512, 1024, 1400]
    cmap = cm.get_cmap('tab10')
    csv_names = [
        'udp_bw_dpdk_fwd_kni_1core.csv',
        'udp_bw_dpdk_fwd_kni_2core.csv',
        'udp_bw_xdp_dpdk_fwd.csv'
    ]

    N = 4
    ind = np.arange(N)    # the x locations for the groups
    width = 0.25         # the width of the bars
    fig, ax = plt.subplots()

    labels = [
        "Centralized Approach 1 vCPU",
        "Centralized Approach 2 vCPU",
        "Chain-based Approach"

    ]
    markers = ['o', '^', 'x']
    line_styles = ['-', '-.', '--']

    for idx, csv in enumerate(csv_names):
        csv_path = os.path.join("./bandwidth/results/", csv)
        bw_arr = np.genfromtxt(csv_path, delimiter=',',
                               usecols=list(range(0, 2))) / 1000.0
        bw_avg = bw_arr[:, 1]
        # bar = ax.bar(ind + idx * width, bw_avg, width, color=cmap(idx), bottom=0,
        #              edgecolor='black', alpha=0.8, lw=0.6,
        #              hatch=hatch_patterns[idx],
        #              label=labels[idx])
        # label_bar(bar, ax)

        ax.plot(ind, bw_avg, color=cmap(idx), ls=line_styles[idx],
                label=labels[idx], marker=markers[idx], markerfacecolor="None",
                markeredgecolor=cmap(idx), ms=3)

    ax.set_ylabel('Bandwidth (Mbits/sec)')

    ax.set_xticks(ind)
    ax.set_xticklabels(('256', '512', '1024', '1400'))
    ax.set_xlabel('Payload Size (Bytes)')
    # ax.set_ylim(0, 35)

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc='upper left')
    # ax.autoscale_view()
    ax.grid(linestyle='--')

    save_fig(fig, "./bandwidth")


if __name__ == '__main__':
    if len(sys.argv) == 3:
        FIG_FMT = sys.argv[2]
    elif sys.argv[1] == 'bw':
        plot_bw()
    else:
        raise RuntimeError('Unknown option!')
