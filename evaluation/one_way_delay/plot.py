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


def calc_owd_lst(subdir, csv_tpl, var_lst):
    """Calculate the OWD"""
    owd_result_lst = list()
    for var in var_lst:
        tmp_list = list()
        csv_name = csv_tpl % (var)
        csv_path = os.path.join(subdir, csv_name)
        owd_arr = np.genfromtxt(csv_path, delimiter=',') / 1000.0
        owd_arr = owd_arr[:, WARM_UP_NUM:]
        tmp_list = np.average(owd_arr, axis=1)
        owd_result_lst.append(
            # Average value and confidence interval
            (np.average(tmp_list), calc_hwci(tmp_list, confidence=0.99))
        )
    print(owd_result_lst)
    return owd_result_lst


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
        csv_path = os.path.join('./results/owd/slp_data', csv_name)
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
        csv_path = os.path.join('./results/owd/cpu_data', csv_name)
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


def plot_ipd():
    tex.setup(width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
              params={})
    ipd_lst = [1, 2, 3, 4, 5, 10, 20]
    cmap = cm.get_cmap('tab10')
    techs = [x+'_%sms.csv' for x in ('kern', 'dpdkfwd', 'dpdkts')]
    labels = ('Kernel IP Forwarding', 'DPDK Forwarding',
              'DPDK Appending Timestamps')
    colors = [cmap(x) for x in range(len(techs))]

    fig, owd_ax = plt.subplots()
    for tech, label, color in zip(techs, labels, colors):
        owd_result_lst = calc_owd_lst('./results/owd/ipd_data/', tech, ipd_lst)

        owd_ax.errorbar(ipd_lst, [t[0] for t in owd_result_lst],
                        yerr=[t[1] for t in owd_result_lst],
                        marker='o', markerfacecolor='None', markeredgewidth=1,
                        markeredgecolor=color,
                        color=color, ecolor='red',
                        label=label, linestyle='--'
                        )

    owd_ax.set_ylabel('One Way Delay (ms)')
    # base_ax.set_xscale('symlog')
    owd_ax.set_xlabel('Probing Interval (ms)')
    owd_ax.set_xticks(ipd_lst)
    handles, labels = owd_ax.get_legend_handles_labels()
    owd_ax.legend(handles, labels, loc='best')
    owd_ax.grid(linestyle='--')
    save_fig(fig, './ipd')


if __name__ == '__main__':
    if len(sys.argv) == 3:
        FIG_FMT = sys.argv[2]
    if sys.argv[1] == 'pi':
        plot_poll_interval()
    elif sys.argv[1] == 'ipd':
        plot_ipd()
    else:
        plot_poll_interval()
