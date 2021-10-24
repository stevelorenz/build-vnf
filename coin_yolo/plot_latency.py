#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
About: Script to plot latency distributions according to percentiles.
Major hints for the plot and hdrh dump from:
https://stackoverflow.com/questions/42072734/percentile-distribution-graph/42094213
"""

import os
from numpy.lib.function_base import percentile
from numpy.lib.histograms import histogram
import probscale
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from hdrh.histogram import HdrHistogram
from hdrh.dump import dump


USEC_PER_SEC = 1000000
MIN_LATEMCY_USECS = 1
MAX_LATENCY_USECS = 24 * 60* 60 *USEC_PER_SEC
LATENCY_SIGNIFICANT_DIGITS = 5
TICKS_PER_HALF_DISTANCE = 5

PORT2IDX = {
    "9999": 0,
    "11111": 1,
}

def getCsvData(path, results, num_rows, duration=None, mps=None):
    for file in os.listdir(path):
        filename = os.fsdecode(file)
        file = path + filename
        
        if file.endswith(".log"):
            data = np.genfromtxt(file, 
                                 delimiter=", ",
                                 skip_header=21,
                                 skip_footer=1,
                                 )
            
            results[:,PORT2IDX[filename.split("_")[1]]] = data[:num_rows,3]

        elif file.endswith(".csv"):
            data = np.genfromtxt(file,
                                 delimiter=","
                                 )

            results[:,0] = data[:num_rows]


def makeFancyPlot(data):
    fig, ax = plt.subplots(figsize=(6, 3))
    fig = probscale.probplot(data, ax=ax, plottype="pp", xscale="log",
                             scatter_kws=dict(linestyle="-"))
    # plt.plot(data, linestyle="-")
    plt.show()

def dumpHdrhLog(file, data):
    histogram = HdrHistogram(MIN_LATEMCY_USECS, MAX_LATENCY_USECS, LATENCY_SIGNIFICANT_DIGITS)

    for d in data:
        histogram.record_value(d)

    histogram.output_percentile_distribution(open(file, "wb"), USEC_PER_SEC, TICKS_PER_HALF_DISTANCE)

# Latencies should be sorted
def dumpPercentilePerValue(file, latencies, percentiles):
    buf = np.asarray((latencies, percentiles))
    np.savetxt(file, buf.T, delimiter=",")

def getXtickLables(num_intervals):
    x_values = 1.0 - 1.0/10**np.arange(0, num_intervals+2)
    lengths = [1,2,2] + [int(v)+1 for v in list(np.arange(3, num_intervals+2))]
    labels = [str(100*v)[0:l] + "%" for v, l in zip(x_values, lengths)]
    labels.append("Max")
    labels.reverse()

    return labels

# @data1: RTTs of the sockperf test
# @data2: Interference durations on the server
def makePercentilePlot(data1, data2, legend, savepath):
    fig, ax = plt.subplots(figsize=(10,8))
    ax.set_xscale("log")
    ax.set_xlim((10e-6, 10e1))
    ax.grid(True, linewidth=0.5, zorder=5)
    ax.grid(True, which="minor", linewidth=0.5, linestyle=":")
    plt.gca().invert_xaxis()

    # Placing the tick labels at the correct positions requires some magic
    ax.set_xticks([10e-6, 10e-5, 10e-4, 10e-3, 10e-2, 10e-1, 10e0, 10e1])
    ax.xaxis.set_ticklabels(getXtickLables(5))

    for plot in range(0, data1.shape[1], 1):
        count, bins_count = np.histogram(data1[:, plot], bins=data1.shape[0])
        pdf = count / sum(count)
        cdf = np.cumsum(pdf) * 100

        ax.plot([100.0 - v for v in cdf], bins_count[1:])

        # dumpPercentilePerValue("new_dump.csv", bins_count[1:], cdf)

    # for plot in range(0, data2.shape[1], 1):
        # count, bins_count = np.histogram(data2[:,plot], bins=data2.shape[0])
        # pdf = count / sum(count)
        # cdf = np.cumsum(pdf) * 100

        # ax.plot([100.0 - v for v in cdf], bins_count[1:])

    
    plt.xlabel("Percentile")
    plt.ylabel("Latency [µs]")
    plt.legend(legend, ncol=2, loc="lower center", bbox_to_anchor=[0.5, -0.15])
    plt.tight_layout()
    plt.savefig(savepath)
    plt.close()

# @data1: RTTs of the sockperf test
# @data2: Interference durations on the server
def makeCdfPlot(data1, data2, legend, savepath):
    f = plt.figure(figsize=(10,8))
    plt.grid()

    for plot in range(0, data1.shape[1], 1):
        count, bins_count = np.histogram(data1[:, plot], bins=data1.shape[0])
        pdf = count / sum(count)
        cdf = np.cumsum(pdf)
        plt.plot(bins_count[1:], cdf)

    for plot in range(0, data2.shape[1], 1):
        count, bins_count = np.histogram(data2[:,plot], bins=data2.shape[0])
        pdf = count / sum(count)
        cdf = np.cumsum(pdf)
        plt.plot(bins_count[1:], cdf)
    
    # plt.xlim((4040, 4100))
    plt.ylabel("CDF")
    plt.xlabel("Latency [µs]")
    plt.legend(legend, ncol=3, loc="lower center", bbox_to_anchor=[0.5, -0.15])
    plt.tight_layout()
    plt.savefig(savepath)
    plt.close()



if __name__ == "__main__":
    path_sockperf = "./measurements/server_local/sockperf/"
    path_interference = "./measurements/server_local/interference/"
    savepath = "./plots/hdrh_latency.pdf"
    savepath_hdrh_dump = "./hdrh_dump/"
    legend = ["SFC", "Default"]#, "Server local"]

    # Sockperf data
    num_packets = 595500
    rtt = np.zeros((num_packets, 2))
    getCsvData(path_sockperf, rtt, num_packets)

    # Server local interference data    
    num_iterations = 30
    duration = np.zeros((num_iterations, 1))
    getCsvData(path_interference, duration, num_iterations)
    duration = duration * 10**6

    makePercentilePlot(rtt, duration, legend, savepath)

    # dumpHdrhLog(savepath_hdrh_dump+"sfc.csv", rtt[:,0])
    # dumpHdrhLog(savepath_hdrh_dump+"default.csv", rtt[:,1])
    # dumpHdrhLog(savepath_hdrh_dump+"server_local.csv", duration)
