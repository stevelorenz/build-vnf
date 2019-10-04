#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8


"""
About: Plot percentile of latencies of YOLO Pre-processing VNF
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.pyplot import cm
import sys

sys.path.append("../scripts/")

CMAP = cm.get_cmap("tab10")

if __name__ == "__main__":
    import tex

    tex.setup(width=1, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17)

    fig, ax = plt.subplots()
    x = np.linspace(50, 100, 1000)

    lines = [
        {
            "color": CMAP(1),
            "ls": "-",
            "label": "Total delay (in VM)",
            "file": "./total_delay_ms_vm.csv",
            "y": None,
        }
    ]

    for l in lines:
        l["y"] = np.genfromtxt(l["file"], delimiter=",").flatten()
        l["y"] = l["y"][~np.isnan(l["y"])]
        l["avg"] = np.average(l["y"])
        l["std"] = np.std(l["y"])

    for l in lines:
        y = l["y"]
        ax.plot(
            x,
            np.percentile(y, x, interpolation="nearest"),
            label=l["label"],
            linestyle=l["ls"],
            color=l["color"],
        )

    ax.set(
        title="Latency of YOLO Pre-processing VNF",
        xlabel="Percentile",
        ylabel="Latency (ms)",
    )

    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc="upper left")

    tex.save_fig(fig, "latency_percentile_50_100", "pdf")
    print(lines)
