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
FIG_FMT = 'png'


def calc_hwci(data, confidence=0.95):
    hwci = stats.sem(data) * stats.t._ppf((1+confidence)/2.0, len(data)-1)
    return hwci


def save_fig(fig, path, fmt=FIG_FMT):
    """Save fig to path"""
    fig.savefig(path + '.%s' % fmt, pad_iches=0,
                bbox_inches='tight', dpi=400, format=fmt)


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


def plot_poll_interval():
    slp_lst = [0]
    slp_lst.extend([10 ** x for x in range(0, 6)])
    # Calc
    owd_result_lst = list()
    for slp in slp_lst:
        tmp_list = list()
        if slp == 0:
            csv_name = 'slp30_0_0_%s.csv' % (slp)
        else:
            csv_name = 'slp30_10_10_%s.csv' % (slp)
        csv_path = os.path.join('./results/slp_data', csv_name)
        owd_arr = np.genfromtxt(csv_path, delimiter=',') / 1000.0
        owd_arr = owd_arr[:, WARM_UP_NUM:]
        tmp_list = np.average(owd_arr, axis=1)
        owd_result_lst.append(
            # Average value and confidence interval
            (np.average(tmp_list), calc_hwci(tmp_list, confidence=0.99))
        )
    print(owd_result_lst)

    # CPU usage
    cpu_usage_lst = list()
    for slp in slp_lst[1:]:
        csv_name = 'cpu30_10_10_%s.csv' % (slp)
        csv_path = os.path.join('./results/cpu_data', csv_name)
        cpu_arr = np.genfromtxt(csv_path, delimiter=',',
                                usecols=range(0, 545))
        cpu_arr = cpu_arr[:, :-1]
        tmp_list = np.average(cpu_arr, axis=1)
        cpu_usage_lst.append(
            (np.average(tmp_list), calc_hwci(tmp_list, confidence=0.99))
        )
    print(cpu_usage_lst)
    cpu_usage_lst.insert(0, (100.0, 0))

    # Plot
    tex.setup(width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
              params={})

    fig, base_ax = plt.subplots()
    owd_ax = base_ax
    cpu_ax = owd_ax.twinx()

    x = slp_lst
    owd_ax.errorbar(x, [t[0] for t in owd_result_lst],
                    yerr=[t[1] for t in owd_result_lst],
                    marker='o', markerfacecolor='None', markeredgewidth=1,
                    markeredgecolor='blue',
                    color='blue', ecolor='red',
                    label='One Way Delay', linestyle='--'
                    )

    owd_ax.set_ylabel('One Way Delay (ms)')

    cpu_ax.errorbar(x, [t[0] for t in cpu_usage_lst],
                    yerr=[t[1] for t in cpu_usage_lst],
                    marker='o', markerfacecolor='None', markeredgewidth=1,
                    markeredgecolor='green',
                    color='green', ecolor='red',
                    label='CPU Usage', linestyle='--'
                    )
    cpu_ax.set_ylabel('CPU Usage (percent)')

    base_ax.set_xscale('symlog')
    base_ax.set_xlabel('Polling Interval (us)')

    for ax in (owd_ax, cpu_ax):
        handles, labels = ax.get_legend_handles_labels()
        ax.legend(handles, labels, loc='best')

    base_ax.set_yscale('symlog')

    save_fig(fig, './dpdk_poll_interval')


if __name__ == '__main__':
    if len(sys.argv) == 3:
        FIG_FMT = sys.argv[2]
    if sys.argv[1] == 'pi':
        plot_poll_interval()
    else:
        plot_poll_interval()
