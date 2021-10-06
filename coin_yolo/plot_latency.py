#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import probscale
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt

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
    path_sockperf = "./results/server_local/sockperf/"
    path_interference = "./results/server_local/interference/"
    savepath = "./plots/latency.pdf"
    legend = ["SFC", "Default", "Server local"]

    # Sockperf data
    num_packets = 595500
    rtt = np.zeros((num_packets, 2))
    getCsvData(path_sockperf, rtt, num_packets)

    # Server local interference data    
    num_iterations = 30
    duration = np.zeros((num_iterations, 1))
    getCsvData(path_interference, duration, num_iterations)
    duration = duration * 10**6

    makeCdfPlot(rtt, duration, legend, savepath)
