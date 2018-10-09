#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Plot evaluation results
"""

import json
import os
import sys

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
sys.path.append('./scripts/')
import tex
from matplotlib.pyplot import cm
from scipy import stats


FIG_FMT = 'pdf'

STYLE_MAP = {
    'Direct Forwarding': {'color': 0, 'ls': '-.'},
    # "Centralized 1 vCPU": {'color': 1, 'ls': ':'},
    "Centralized": {'color': 4, 'ls': 'dashed'},
    "Chain-based": {'color': 2, 'ls': 'solid'},
}

CMAP = cm.get_cmap('tab10')


def calc_hwci(data, confidence=0.95):
    hwci = stats.sem(data) * stats.t._ppf((1+confidence)/2.0, len(data)-1)
    return hwci


def calc_std_err(data):
    return stats.sem(data)


def save_fig(fig, path, fmt=FIG_FMT):
    """Save fig to path"""
    fig.savefig(path + '.%s' % fmt, pad_iches=0,
                bbox_inches='tight', dpi=1200, format=fmt)


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
        'udp_bw_dpdk_fwd_kni_2core.csv',
        'udp_bw_xdp_dpdk_fwd.csv'
    ]

    N = 4
    ind = np.arange(N)    # the x locations for the groups
    width = 0.25         # the width of the bars
    fig, ax = plt.subplots()

    labels = [
        "Centralized",
        "Chain-based"

    ]
    markers = ['o', 'x', '^']
    line_styles = ['dashed', 'solid', '--']

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

        ax.plot(ind, bw_avg, color=cmap(STYLE_MAP[labels[idx]]['color']), ls=line_styles[idx],
                label=labels[idx], marker=markers[idx], markerfacecolor="None",
                markeredgecolor=cmap(STYLE_MAP[labels[idx]]['color']), ms=3)

    ax.set_ylabel('Bandwidth (Mbits/sec)')

    ax.set_xticks(ind)
    ax.set_xticklabels(('256', '512', '1024', '1400'))
    ax.set_xlabel('Payload Size (Bytes)')

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc='upper left')
    ax.autoscale_view()
    ax.grid(linestyle='--')

    save_fig(fig, "./bandwidth")


def plot_adv_comp_per_packet_delay(payload_size=1400):
    tex.setup(width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17,
              params={
                  'hatch.linewidth': 0.5
              }
              )
    print("Current payload size: {}B".format(payload_size))
    N = 9
    if payload_size == 256:
        N = 9
    ind = np.arange(N)
    width = 0.15
    fig, ax = plt.subplots()

    # Interate different adv functions
    func_map = {
        "nc_enc_sliding_window_25_binary8": [0, "NC Sliding Window", "++++",
                                             "-", "o"],
        "nc_enc_noack_25_binary8": [1, "NC Block Code", "xxxx", "--", "x"],
        "aes_ctr_enc_25_256": [2, "AES256 ENC", "\\\\\\\\", "-.", "+"],
        "aes_ctr_dec_25_256": [3, "AES256 DEC", "////", ":", "v"],
    }
    try:
        for func, para in func_map.items():
            csv_files = list()
            for i in range(1, N+1):
                if payload_size != 1400:
                    csv_files.append(
                        "{}_per_packet_delay_{}_{}.csv".format(i, func,
                                                               payload_size))
                else:
                    csv_files.append(
                        "{}_per_packet_delay_{}.csv".format(i, func))
            delay_map = dict()
            delay_map_val = list()
            for csv in csv_files:
                csv_path = os.path.join("./comp_eval/results/", csv)
                delay_arr = np.genfromtxt(csv_path, delimiter=',') * 1000.0
                delay_arr_tmp = delay_arr[:] / 1000.0
                delay_arr = delay_arr.flatten()
                delay_arr = delay_arr[500:-500]
                delay_arr = delay_arr[0:50000]
                if csv == '8_per_packet_delay_aes_ctr_dec_25_256_256.csv':
                    delay_arr = [x for x in delay_arr if x < 20000]
                delay_map_val.append(
                    (np.average(delay_arr), calc_std_err(delay_arr)))
            print("{}, data array:{}".format(func, json.dumps(delay_map_val)))
            vals = delay_map_val[:]

            ax.bar(ind + para[0] * width, [x[0] for x in vals], width,
                   edgecolor='black', lw=0.6, alpha=0.8,
                   ecolor='black', error_kw={"elinewidth": 1},
                   hatch=para[2], label=para[1],
                   color=CMAP(para[0]), yerr=[x[1] for x in vals])

            # ax.errorbar(
            #     ind, [x[0] for x in vals], color=CMAP(para[0]), ls=para[3],
            #     ecolor="red", yerr=[x[1] for x in vals], label=para[1], marker=para[4], markerfacecolor="None",
            #     markeredgecolor=CMAP(para[0]), ms=3
            # )

    except Exception as e:
        print(e)

    ax.axhline(y=20, ls="--", color='black', lw=1)

    if payload_size == 256:
        ax.set_ylim(0, 30)
    else:
        # ax.set_yscale('log')
        ax.set_ylim(1, 100)
        # yticks = [1, 10, 20, 100]
        # ax.set_yticks(yticks)
    ax.set_xlabel("Number of VNF(s)")
    ax.set_ylabel('Per-packet Processing Delay (us)')
    # ax.set_xticks(ind)
    ax.set_xticks(ind + 3 * width / 2.0)
    ax.set_xticklabels(list(map(str, range(1, N+1))))
    # ax.set_title("Payload size: {} Bytes".format(payload_size))

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc='upper left', ncol=2)
    ax.autoscale_view()
    ax.grid(ls='--')
    save_fig(fig, "./adv_comp_eval_{}".format(payload_size))


if __name__ == '__main__':
    if len(sys.argv) == 3:
        FIG_FMT = sys.argv[2]
    elif sys.argv[1] == 'bw':
        plot_bw()
    elif sys.argv[1] == 'adv':
        plot_adv_comp_per_packet_delay(1400)
    elif sys.argv[1] == 'adv256':
        plot_adv_comp_per_packet_delay(256)
    else:
        raise RuntimeError('Unknown option!')
