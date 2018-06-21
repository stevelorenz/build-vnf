#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Plot results of round trip time measurements.
"""

import os
import sys
sys.path.append('../scripts/')
import tex

import ipdb
import numpy as np

import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.pyplot import cm
from scipy import stats

WARM_UP_NUM = 10
TOTAL_PACK_NUM = 500
FIG_FMT = 'png'


def calc_hwci(data, confidence=0.95):
    hwci = stats.sem(data) * stats.t._ppf((1+confidence)/2.0, len(data)-1)
    return hwci


def save_fig(fig, path, fmt=FIG_FMT):
    """Save fig to path"""
    fig.savefig(path + '.%s' % fmt, pad_iches=0,
                bbox_inches='tight', dpi=400, format=fmt)


def calc_rtt_lst(subdir, csv_tpl, var_lst):
    rtt_result_lst = list()
    for var in var_lst:
        tmp_list = list()
        csv_name = csv_tpl % (var)
        csv_path = os.path.join(subdir, csv_name)
        rtt_arr = np.genfromtxt(csv_path, delimiter=',',
                                usecols=list(range(0, TOTAL_PACK_NUM))) / 1000.0
        rtt_arr = rtt_arr[:, WARM_UP_NUM:]
        # Check nagative values
        signs = np.sign(rtt_arr).flatten()
        neg_sign = -1
        if neg_sign in signs:
            raise RuntimeError('Found negative RTT value in %s.' % csv_path)
        tmp_list = np.average(rtt_arr, axis=1)
        rtt_result_lst.append(
            # Average value and confidence interval
            (np.average(tmp_list), calc_hwci(tmp_list, confidence=0.99))
        )
    print(rtt_result_lst)
    return rtt_result_lst


def label_bar(rects, ax):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2. + 0.05, 1.05*height,
                '%.3f' % height,
                ha='center', va='bottom')


def plot_ipd():
    tex.setup(width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
              params={
                  'hatch.linewidth': 0.5
              }
              )
    ipd_lst = [5]
    cmap = cm.get_cmap('tab10')
    techs = ['udp_rtt_' + x +
             '_%sms.csv' for x in (
                 'lkfwd', 'click_fwd', 'dpdk_fwd', 'dpdk_appendts')
             ]
    xtick_labels = ('Kernel Forwarding', 'Click Forwarding',
                    'DPDK Forwarding', 'DPDK Timestamps', 'Click Forwarding')
    colors = [cmap(x) for x in range(len(techs))]
    hatch_patterns = ('xxx', '///', '+++', '\\\\\\', 'ooo', 'OOO', '...')
    bar_width = 0.10
    gap = 0.05
    fig, rtt_ax = plt.subplots()
    for idx, tech in enumerate(techs):
        rtt_result_lst = calc_rtt_lst('./results/rtt/', techs[idx], ipd_lst)
        rtt_bar = rtt_ax.bar([0 + idx * (bar_width + gap)], [t[0] for t in rtt_result_lst],
                             yerr=[t[1] for t in rtt_result_lst],
                             color=colors[idx], ecolor='red',
                             hatch=hatch_patterns[idx],
                             width=bar_width)
        label_bar(rtt_bar, rtt_ax)

    rtt_ax.set_ylabel('Round Trip Time (ms)')
    rtt_ax.set_xticks([0 + x * (bar_width + gap) for x in range(len(techs))])
    # rtt_ax.set_xticks([0, 0+bar_width+gap])
    rtt_ax.set_ylim(0, )
    rtt_ax.set_xticklabels(xtick_labels, fontsize=3)
    rtt_ax.grid(linestyle='--')
    save_fig(fig, './rtt_fix_ipd')


if __name__ == '__main__':
    if len(sys.argv) == 3:
        FIG_FMT = sys.argv[2]
    elif sys.argv[1] == 'ipd':
        plot_ipd()
    else:
        raise RuntimeError('Unknown option!')
