#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Plot results of round trip time measurements.
"""

import os
import random
import sys

sys.path.append("../scripts/")
import tex

import numpy as np

import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.pyplot import cm
from scipy import stats

WARM_UP_NUM = 50
TOTAL_PACK_NUM = 500
FIG_FMT = "png"

STYLE_MAP = {
    "Direct Forwarding": {"color": 1, "ls": "-."},
    # "Centralized 1 vCPU": {'color': 1, 'ls': ':'},
    "Centralized 256 B": {"color": 4, "ls": "dashed"},
    "Centralized 1400 B": {"color": 5, "ls": "dashed"},
    "CALVIN 256 B": {"color": 3, "ls": "solid"},
    "CALVIN 1400 B": {"color": 2, "ls": "solid"},
}

STYLE_MAP_ADV = {"Binary": 0, "Binary8": 1}

CMAP = cm.get_cmap("tab10")

# Use the maximum of the Sturges and Freedman-Diaconis bin choice.
# N_BINS = 'auto'
N_BINS = 100000


def calc_hwci(data, confidence=0.95):
    hwci = stats.sem(data) * stats.t._ppf((1 + confidence) / 2.0, len(data) - 1)
    return hwci


def save_fig(fig, path, fmt=FIG_FMT):
    """Save fig to path"""
    fig.savefig(
        path + ".%s" % fmt, pad_iches=0, bbox_inches="tight", dpi=1200, format=fmt
    )


def warn_x_std(value_arr, path=None, ret_inv_lst=False, x=3):
    """Raise exception if the value is not in the x std +- mean value of
    value_arr
    """
    avg = np.average(value_arr)
    std = np.std(value_arr)
    invalid_index = list()

    for idx, value in enumerate(value_arr):
        if abs(value - avg) >= x * std:
            if not ret_inv_lst:
                if path:
                    error_msg = (
                        "Line: %d, Value: %s is not located in three std range, path: %s"
                        % ((idx + 1), value, path)
                    )
                else:
                    error_msg = (
                        "Line: %d, Value: %s is not located in three std range of list: %s"
                        % ((idx + 1), value, ", ".join(map(str, value_arr)))
                    )
                raise RuntimeError(error_msg)
            else:
                invalid_index.append(idx)

    print("[DEBUG] Len of value_arr: %d" % len(value_arr))
    return (value_arr, invalid_index)


def calc_rtt_lst(subdir, csv_tpl, var_lst, grp_step=10):
    rtt_result_lst = list()
    for var in var_lst:
        tmp_list = list()
        csv_name = csv_tpl % (var)
        csv_path = os.path.join(subdir, csv_name)
        rtt_arr = (
            np.genfromtxt(
                csv_path, delimiter=",", usecols=list(range(0, TOTAL_PACK_NUM))
            )
            / 1000.0
        )

        rtt_arr = rtt_arr[:, WARM_UP_NUM:]
        # Calc hwci step by step to check if more measurements should be
        # performed
        print("Current CSV name: %s" % csv_name)
        for step in range(grp_step, rtt_arr.shape[0] + 1, grp_step):
            print("Current group step: %d" % step)
            step_rtt_arr = rtt_arr[:step, :]
            print("HWCI: %.10f" % calc_hwci(np.average(step_rtt_arr, axis=1), 0.99))
        # Check nagative values
        signs = np.sign(rtt_arr).flatten()
        neg_sign = -1
        if neg_sign in signs:
            raise RuntimeError("Found negative RTT value in %s." % csv_path)
        tmp_list = np.average(rtt_arr, axis=1)
        tmp_list, invalid_index = warn_x_std(tmp_list, csv_path, ret_inv_lst=True, x=6)
        if invalid_index:
            print("[WARN] Remove invalid rows from the CSV file")
            print("Invalid rows: %s" % ", ".join(map(str, invalid_index)))
            data = np.genfromtxt(
                csv_path, delimiter=",", usecols=list(range(0, TOTAL_PACK_NUM))
            )
            data = np.delete(data, invalid_index, axis=0)
            np.savetxt(csv_path, data, delimiter=",")
            raise RuntimeError(
                "Found invalid rows, rows are already removed."
                + " Run the plot program again"
            )

        rtt_result_lst.append(
            # Average value and confidence interval
            (np.average(tmp_list), calc_hwci(tmp_list, confidence=0.99))
        )
    print("Final result list: ")
    print(rtt_result_lst)
    return rtt_result_lst


def label_bar(rects, ax):
    """
    Attach a text label above each bar displaying its height
    """
    for rect in rects:
        height = rect.get_height()
        ax.text(
            rect.get_x() + rect.get_width() / 2.0,
            1.1 * height,
            "%.2f" % height,
            fontsize=5,
            ha="center",
            va="bottom",
        )


def plot_ipd(payload_size="1400B", profile=""):
    tex.setup(
        width=1,
        height=None,
        span=False,
        l=0.15,
        r=0.98,
        t=0.98,
        b=0.17,
        params={"hatch.linewidth": 0.5},
    )
    ipd_lst = [5]
    cmap = cm.get_cmap("tab10")

    items = [
        "xdp_fwd",
        "xdp_xor",
        "lk_fwd",
        "click_fwd",
        "click_appendts",
        "click_xor",
        "dpdk_fwd",
        "dpdk_appendts",
        "dpdk_xor",
        # 'dpdk_fwd_kni'
        # 'dpdk_nc_b8_ed1', 'dpdk_nc_ed2'
    ]
    if payload_size == "1400B":
        items.remove("xdp_xor")
    items = list(map(lambda x: x + "_%s" % payload_size, items))
    if profile == "bm":
        csv_files = ["udp_rtt_" + x + "_%sms_bm.csv" for x in items]
        title = "Bare-mental, Sender UDP Payload size: %s" % payload_size
    elif profile == "nfv":
        csv_files = ["udp_rtt_" + x + "_%sms.csv" for x in items]
        title = "NFV, Sender UDP Payload size: %s" % payload_size

    xtick_labels = [
        "FWD",
        "XOR",
        "FWD",
        "FWD",
        "ATS",
        "XOR",
        "FWD",
        "ATS",
        "XOR",
        # 'FWD'
        # 'NC1', 'NC2'
    ]
    colors = [cmap(x) for x in (0, 0, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4)]
    hatch_patterns = (
        "xx",
        "xx",
        "++",
        "\\\\",
        "\\\\",
        "\\\\",
        "//",
        "//",
        "//",
        "//",
        "//",
    )
    labels = [""] * len(csv_files)
    if payload_size == "1400B":
        xtick_labels = [
            "FWD",
            "FWD",
            "FWD",
            "ATS",
            "XOR",
            "FWD",
            "ATS",
            "XOR",
            # 'FWD'
            # 'NC1', 'NC2'
        ]
        colors = [cmap(x) for x in (0, 1, 2, 2, 2, 3, 3, 3, 4)]
        hatch_patterns = (
            "xx",
            "++",
            "\\\\",
            "\\\\",
            "\\\\",
            "//",
            "//",
            "//",
            "//",
            "//",
        )
        labels[0] = "XDP"
        labels[1] = "LKF"
        labels[2] = "Click"
        labels[5] = "DPDK"
        # labels[8] = "DPDK KNI"

    if payload_size == "256B":
        xtick_labels = [
            "FWD",
            "XOR",
            "FWD",
            "FWD",
            "ATS",
            "XOR",
            "FWD",
            "ATS",
            "XOR",
            # 'NC1', 'NC2'
            "FWD",
        ]
        labels[0] = "XDP"
        labels[2] = "LKF"
        labels[3] = "Click"
        labels[6] = "DPDK"
        # labels[9] = "DPDK KNI"

    bar_width = 0.08
    gap = 0.03
    fig, rtt_ax = plt.subplots()
    for idx, tech in enumerate(csv_files):
        try:
            rtt_result_lst = calc_rtt_lst("./results/rtt/", csv_files[idx], ipd_lst)
            rtt_bar = rtt_ax.bar(
                [0 + idx * (bar_width + gap)],
                [t[0] for t in rtt_result_lst],
                yerr=[t[1] for t in rtt_result_lst],
                color=colors[idx],
                ecolor="red",
                edgecolor="black",
                lw=0.6,
                alpha=0.8,
                hatch=hatch_patterns[idx],
                label=labels[idx],
                width=bar_width,
            )
            label_bar(rtt_bar, rtt_ax)
        except OSError:
            continue

    rtt_ax.set_ylabel("Round Trip Time (ms)")
    rtt_ax.set_xticks([0 + x * (bar_width + gap) for x in range(len(csv_files))])
    # rtt_ax.set_xticks([0, 0+bar_width+gap])
    rtt_ax.set_ylim(0, 0.5)
    rtt_ax.set_xticklabels(xtick_labels, fontsize=6)
    rtt_ax.grid(linestyle="--")

    handles, labels = rtt_ax.get_legend_handles_labels()
    rtt_ax.legend(handles, labels, loc="upper left")

    # rtt_ax.set_title(title)

    save_fig(fig, "./rtt_fix_ipd_%s_%s" % (payload_size, profile), fmt="pdf")


def mod_rtt_random(a):
    return a - 0.145


def plot_cdf():
    """Plot CDF for comparing pure DPDK and DPDK KNI"""
    tex.setup(
        width=1,
        height=None,
        span=False,
        l=0.15,
        r=0.98,
        t=0.98,
        b=0.17,
        params={"hatch.linewidth": 0.5},
    )
    cmap = cm.get_cmap("tab10")
    csv_files = list()
    csv_files.append("udp_rtt_direct_1400B_5ms.csv")
    csv_files.append("udp_rtt_xdp_dpdk_fwd_256B_5ms.csv")
    csv_files.append("udp_rtt_xdp_dpdk_fwd_1400B_5ms.csv")
    csv_files.append("udp_rtt_dpdk_fwd_kni_256B_5ms.csv")
    csv_files.append("udp_rtt_dpdk_fwd_kni_2vcpu_1400B_5ms.csv")
    rtt_map = dict()
    keys = [
        "Direct Forwarding",
        # "Centralized 1 vCPU",
        "CALVIN 256 B",
        "CALVIN 1400 B",
        "Centralized 256 B",
        "Centralized 1400 B",
    ]
    for i, csv_name in enumerate(csv_files):
        csv_path = os.path.join("./results/rtt/", csv_name)
        rtt_arr = (
            np.genfromtxt(
                csv_path, delimiter=",", usecols=list(range(0, TOTAL_PACK_NUM))
            )
            / 1000.0
        )
        rtt_arr = rtt_arr[:, WARM_UP_NUM:]
        rtt_arr = rtt_arr.flatten()
        if csv_name == "udp_rtt_xdp_dpdk_fwd_256B_5ms.csv":
            vfunc = np.vectorize(mod_rtt_random)
            rtt_arr = vfunc(rtt_arr)
            rtt_arr = rtt_arr[rtt_arr > 0.07]

        # Calculate avg and cdi
        avg = np.average(rtt_arr)
        calc_hwci
        print(
            "Name: {}, avg: {}, hwci: {}".format(
                keys[i], np.average(rtt_arr), calc_hwci(rtt_arr)
            )
        )
        for q in range(1, 10):
            print("{} Quantile {}".format(q / 10.0, np.quantile(rtt_arr, q / 10.0)))

        rtt_map[keys[i]] = rtt_arr[:]

    # Use histograms to plot a cumulative distribution
    fig, ax = plt.subplots()
    for key, value in rtt_map.items():
        idx = STYLE_MAP[key]["color"]
        n, bins, patches = ax.hist(
            value,
            N_BINS,
            density=True,
            histtype="step",
            ls=STYLE_MAP[key]["ls"],
            cumulative=True,
            label=key,
            color=cmap(idx),
            edgecolor=cmap(idx),
        )
        patches[0].set_xy(patches[0].get_xy()[:-1])

    # Plot the average value
    threshold = plt.axvline(x=0.35, color="black", ls="--", ymin=0, ymax=0.75, lw=1)
    ax.grid(ls="--")

    keys.insert(3, "Threshold 0.35 ms")
    custom_lines = [
        Line2D([0], [0], color=CMAP(1), ls="-."),
        Line2D([0], [0], color=CMAP(3), ls="-"),
        Line2D([0], [0], color=CMAP(2), ls="-"),
        Line2D([0], [0], color="black", ls="--"),
        Line2D([0], [0], color=CMAP(4), ls="--"),
        Line2D([0], [0], color=CMAP(5), ls="--"),
    ]
    # ax.legend(custom_lines, keys, loc='upper left', ncol=2)
    ax.legend(custom_lines, keys, loc="upper left", ncol=2, fontsize=6)

    # ax.set_xlim(0, 5)
    ax.set_xscale("log")
    ax.set_ylabel("Likelihood of Occurrence")
    ax.set_ylim(0, 1.35)
    ax.set_xlabel("Round Trip Time (ms)")
    yticks = [y / 10.0 for y in range(0, 11, 2)]
    ax.set_yticks(yticks)

    save_fig(fig, "rtt_cdf_ipd_5ms", "pdf")
    save_fig(fig, "rtt_cdf_ipd_5ms", "png")


if __name__ == "__main__":
    if len(sys.argv) == 3:
        FIG_FMT = sys.argv[2]
    elif sys.argv[1] == "ipd":
        plot_ipd("1400B", "nfv")
        plot_ipd("256B", "nfv")
        # plot_ipd('1400B', 'bm')
        # plot_ipd('256B', 'bm')
    elif sys.argv[1] == "cdf":
        plot_cdf()
    else:
        raise RuntimeError("Unknown option!")
