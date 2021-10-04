#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt

PORT2IDX = {
    "9999": 0,
    "11111": 1,
}

def getCsvData(path, results, num_packets, duration=None, mps=None):
    for file in os.listdir(path):
        filename = os.fsdecode(file)
        file = path + filename
        
        check_param = 0
        split_idx = 0
        if duration is not None:
            check_param = duration
            split_idx = 3
        elif mps is not None:
            check_param = mps
            split_idx = 5

        if file.endswith("log"):
            m_idx = int(filename.split("_")[-2])
            m_check = int(filename.split("_")[split_idx])
            if m_idx == 0 and check_param == m_check:
                data = np.genfromtxt(file, 
                                     delimiter=", ",
                                     skip_header=21,
                                     skip_footer=1,
                                     )

                results[:, PORT2IDX[filename.split("_")[1]]] = data[:num_packets, 3]

def makeCdfPlot(data, legend, savepath):
    f = plt.figure(figsize=(10,8))
    plt.grid()

    for plot in range(0, data.shape[1], 1):
        count, bins_count = np.histogram(data[:, plot], bins=data.shape[0])
        pdf = count / sum(count)
        cdf = np.cumsum(pdf)
        plt.plot(bins_count[1:], cdf)
    
    plt.xlim((4040, 4100))
    plt.ylabel("CDF")
    plt.xlabel("Latency [µs]")
    plt.legend(legend, ncol=2, loc="lower center", bbox_to_anchor=[0.5, -0.15])
    plt.tight_layout()
    plt.savefig(savepath)
    plt.close()


if __name__ == "__main__":
    path_duration = "./sockperf_csv/duration/"
    path_mps = "./sockperf_csv/mps/"
    legend = ["SFC", "Default"]

    num_packets_duration = [45500, 95500, 295500, 595500, 1195400]
    num_packets_mps = [5950, 59500, 297700, 595500, 2977400, 5200000]
    test_duration = [5, 10, 30 , 60, 120]
    test_mps = [100, 1000, 5000, 10000, 50000, 100000]

    for test in range(0, len(test_duration), 1):
        savepath = f"./plots/test_duration_{test_duration[test]}_s.png"
        rtt = np.zeros((num_packets_duration[test], 2))

        getCsvData(path_duration, rtt, num_packets_duration[test], duration=test_duration[test])
        makeCdfPlot(rtt, legend, savepath)

    for test in range(0, len(test_mps), 1):
        savepath = f"./plots/test_mps_{test_mps[test]}.png"
        rtt = np.zeros((num_packets_mps[test], 2))
        
        getCsvData(path_mps, rtt, num_packets_mps[test], mps=test_mps[test])
        makeCdfPlot(rtt, legend, savepath)
