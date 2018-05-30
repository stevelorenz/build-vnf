#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Plot results of one-way delay measurements.
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

WARM_UP_NUM = 500


def calc_hwci(data, confidence=0.95):
    hwci = stats.sem(data) * stats.t._ppf((1+confidence)/2.0, len(data)-1)
    return hwci


def save_fig(fig, path):
    """Save fig to path"""
    fig.savefig(path + '.pdf', pad_iches=0,
                bbox_inches='tight', dpi=400, format='pdf')


def plot_dpdk():

    ### Calc ###
    ipd_lst = [1000, 500, 100]
    ipd_lst.extend(range(8, 22, 2))
    ipd_lst.sort()

    owd_result_lst = list()

    for ipd in ipd_lst:
        tmp_list = list()
        csv_name = 'udp_owd_1_%dus.csv' % (ipd)
        csv_path = os.path.join('./result_csv/dpdk', csv_name)
        owd_arr = np.genfromtxt(csv_path, delimiter=',') / 1000.0
        owd_arr = owd_arr[:, WARM_UP_NUM:]
        tmp_list = np.average(owd_arr, axis=1)
        owd_result_lst.append(
            # Average value and confidence interval
            (np.average(tmp_list), calc_hwci(tmp_list))
        )
    print(owd_result_lst)
    ### Plot ###
    tex.setup(width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
              params={})

    fig, ax = plt.subplots()
    x = list(map(lambda x: x / 1000.0, ipd_lst))
    ax.errorbar(x, [t[0] for t in owd_result_lst],
                yerr=[t[1] for t in owd_result_lst],
                color='blue', ecolor='red',
                label='Chain Length = 1', linestyle='--'
                )

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc='best')
    ax.grid(linestyle='--')

    ax.set_xlim(0.0075, 0.020)
    ax.set_xlabel('Probing interval (ms)')
    ax.set_ylabel('One Way Delay (ms)')
    save_fig(fig, './dpdk_udp_ipd')


if __name__ == '__main__':
    plot_dpdk()
