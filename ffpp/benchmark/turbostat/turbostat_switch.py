"""
About: Parse and plot the data from the frequency_switch test
"""

import sys
import os
import math
import numpy as np
import matplotlib.pyplot as plt


def getData(path, power, pos, mask):
    num_file = 0
    for file in os.listdir(path):
        filename = os.fsdecode(file)
        file = path + filename

        if file.endswith("ts"):
            print("File ", num_file, ": ", filename)
            data = np.genfromtxt(file, delimiter=",")[2::mask]
            power[num_file][0] = data[:, pos[0]].copy()
            power[num_file][1] = data[:, pos[1]].copy()
            num_file += 1


def getMean(data):
    mean = np.zeros((data.shape[1], data.shape[2]))

    for i in range(0, data.shape[1], 1):
        for j in range(0, data.shape[2], 1):
            count = 0
            sum = 0
            for val in range(0, data.shape[0], 1):
                if not (math.isnan(data[val, i, j])):
                    count += 1
                    sum += data[val, i, j]
                    if i == 2 and j == 9:
                        print("File: ", val, ", value: ", data[val, i, j])
                else:
                    print("Invalid val: ", val, i, j)
            mean[i, j] = sum / count

    return mean


def parseFreqs(power, margin, freqs):
    iterations = power.shape[0]
    pstates = len(freqs)
    mean_power_up = np.zeros((iterations, 3, pstates))
    mean_power_down = np.zeros((iterations, 3, pstates))

    for it in range(0, iterations, 1):
        # Ini for each file
        freq_idx = 1
        freq = freqs[freq_idx]
        idx = 50  # First measurements are often shitty
        last_freq_idx = idx
        freq_update = False
        transition = False
        first = False

        # First, for increasing frequency
        for i in range(idx, power.shape[2]):
            if not first:
                if power[it, 0, i] == freq:
                    # @-2: To not take transition freqs into account
                    mean_power_up[it, 0, 0] = np.mean(
                        power[it][0][last_freq_idx : i - 2], axis=0
                    )
                    mean_power_up[it, 1, 0] = np.mean(
                        power[it][1][last_freq_idx : i - 2], axis=0
                    )
                    mean_power_up[it, 2, 0] = np.std(
                        power[it][1][last_freq_idx : i - 2], axis=0
                    )
                    last_freq_idx = i
                    first = True
            if first:
                if power[it, 0, i] > freq + margin and not transition:
                    # Get mean power consumption for current frequency
                    mean_power_up[it, 1, freq_idx] = np.mean(
                        power[it][1][last_freq_idx : i - 1], axis=0
                    )
                    mean_power_up[it, 2, freq_idx] = np.std(
                        power[it][1][last_freq_idx : i - 1], axis=0
                    )
                    last_freq_idx = i
                    transition = True
                    freq_update = True

            # Update if condition
            if freq_update:
                mean_power_up[it, 0, freq_idx] = freq
                if freq == freqs[-2]:
                    freq = freqs[-2]
                    idx = i
                    freq_update = False
                    transition = False
                    first = False
                    break
                else:
                    freq_idx += 1
                    freq = freqs[freq_idx]
                mean_power_up[it, 0, freq_idx] = freq
                freq_update = False

            # Check for transition frequencies
            if transition:
                if power[it, 0, i] >= freq - margin:
                    last_freq_idx = i
                    transition = False

        # Then, for decreasing frequency
        for i in range(idx, power.shape[2], 1):
            if not first:
                if power[it, 0, i] == freq:
                    # @-2: To not take transition freqs into account
                    mean_power_down[it, 0, -1] = np.mean(
                        power[it][0][last_freq_idx + 2 : i - 2], axis=0
                    )
                    mean_power_down[it, 1, -1] = np.mean(
                        power[it][1][last_freq_idx + 2 : i - 2], axis=0
                    )
                    mean_power_down[it, 2, -1] = np.std(
                        power[it][1][last_freq_idx + 2 : i - 2], axis=0
                    )
                    last_freq_idx = i
                    first = True
            if first:
                if power[it, 0, i] < freq - margin and not transition:
                    mean_power_down[it, 1, freq_idx] = np.mean(
                        power[it][1][last_freq_idx : i - 1], axis=0
                    )
                    mean_power_down[it, 2, freq_idx] = np.std(
                        power[it][1][last_freq_idx : i - 1], axis=0
                    )
                    last_freq_idx = i
                    transition = True
                    freq_update = True

            # Update if condition
            if freq_update:
                mean_power_down[it, 0, freq_idx] = freq
                freq_idx -= 1
                if freq == freqs[-1]:
                    freq = freqs[-2]
                else:
                    freq = freqs[freq_idx]
                mean_power_down[it, 0, freq_idx] = freq
                freq_update = False

            # Check for transition frequencies
            if transition:
                if power[it, 0, i] <= freq + margin:
                    last_freq_idx = i
                    transition = False

            if freq == freqs[0]:  # and not transition:
                # @+2: To not take transition freqs into account
                offset = -1000  # ignore last measurements
                mean_power_down[it, 0, freq_idx] = np.mean(
                    power[it, 0, last_freq_idx + 2 : offset]
                )
                mean_power_down[it, 1, freq_idx] = np.mean(
                    power[it, 1, last_freq_idx + 2 : offset]
                )
                mean_power_down[it, 2, freq_idx] = np.std(
                    power[it, 1, last_freq_idx + 2 : offset]
                )
                break

    mean_power_up[:, :, -1] = mean_power_down[:, :, -1].copy()

    return mean_power_up, mean_power_down


def makePlot(data, pstates, title=None, ylabel=None, savedir=None):
    # plt.plot(data[0], data[1], '.')
    plt.errorbar(data[0], data[1], yerr=data[2], fmt=".", capsize=2)
    plt.grid()
    plt.title(title)
    plt.xlabel("frequency [MHz]")
    plt.ylabel(ylabel)
    ticks = ["%.2f" % tick for tick in data[0]]
    # plt.xticks((ticks))
    # plt.xticks(data[0])
    if savedir:
        plt.savefig(savedir)
        plt.close()
    else:
        plt.show()


def makePlotComb(data1, data2, title=None, ylabel=None, legend=None, savedir=None):
    plt.errorbar(data1[0], data1[1], yerr=data1[2], fmt="b.", ecolor="b", capsize=2)
    plt.errorbar(data2[0], data2[1], yerr=data2[2], fmt="g.", ecolor="g", capsize=2)
    plt.grid()
    plt.title(title)
    plt.xlabel("frequency [MHz]")
    plt.ylabel(ylabel)
    plt.legend(legend)
    # plt.xticks(data[0])
    if savedir:
        plt.savefig(savedir)
        plt.close()
    else:
        plt.show()


if __name__ == "__main__":
    # global params
    INTEL = [
        800,
        900,
        1000,
        1100,
        1200,
        1300,
        1400,
        1500,
        1600,
        1700,
        1800,
        1900,
        2000,
        2100,
        2200,
        2300,
        2400,
        2500,
        2600,
        2700,
        2800,
        2900,
        3000,
        3100,
        3200,
        3300,
        3400,
        3700,
    ]
    ACPI = [
        800,
        1000,
        1200,
        1400,
        1500,
        1700,
        1900,
        2100,
        2300,
        2500,
        2700,
        2800,
        3000,
        3200,
        3400,
        3700,
    ]

    base_savedir = "./turbostat_stats/plots/frequency_switch_"
    mask = 3
    pos = [0, 1]
    margin = 5  # to allow frequency deviations

    # # --- Plot single driver ---
    # # Driver params
    # path = './turbostat_stats/switch/acpi/'
    # samples = 4000
    # iterations = 99

    # power = np.zeros((iterations, 2, samples))
    # getData(path, power, pos, mask)

    # if path.count('intel'):
    # power_up, power_down = parseFreqs(power, margin, INTEL)
    # power_up[:, 0, 0] = 800
    # power_down[:, 0, 0] = 800
    # elif path.count('acpi'):
    # power_up, power_down = parseFreqs(power, margin, ACPI[4:])

    # # mean_power_up = np.mean(power_up, axis=0)
    # # mean_power_down = np.mean(power_down, axis=0)
    # mean_power_up = getMean(power_up)
    # mean_power_down = getMean(power_down)

    # # print('Up\n', power_up)
    # # print('Down\n', power_down)
    # print('Up\n', mean_power_up)
    # print('Down\n', mean_power_down)

    # savedir_up = base_savedir + path.split('/')[-2] + '_increase.png'
    # savedir_down = base_savedir + path.split('/')[-2] + '_decrease.png'
    # print(savedir_up)
    # print(savedir_down)

    # makePlot(mean_power_down, 28,
    # title='Frequency Decrease',
    # ylabel='power consumption [W]',
    # savedir=savedir_down)

    # makePlot(mean_power_up, 27,
    # title='Frequency Increase',
    # ylabel='power consumption [W]',
    # savedir=savedir_up)

    # --- Plot both intel and acpi together ---
    # intel params
    path_intel = "./turbostat_stats/switch/intel/"
    samples_intel = 6300
    iterations_intel = 100

    # acpi params
    path_acpi = "./turbostat_stats/switch/acpi/"
    samples_acpi = 4000
    iterations_acpi = 99

    # Get data
    power_intel = np.zeros((iterations_intel, 2, samples_intel))
    power_acpi = np.zeros((iterations_acpi, 2, samples_acpi))
    getData(path_intel, power_intel, pos, mask)
    getData(path_acpi, power_acpi, pos, mask)

    # Parse data
    power_up_intel, power_down_intel = parseFreqs(power_intel, margin, INTEL)
    power_up_acpi, power_down_acpi = parseFreqs(power_acpi, margin, ACPI[4:])
    power_up_intel[:, 0, 0] = 800
    power_down_intel[:, 0, 0] = 800
    mean_power_up_intel = np.mean(power_up_intel, axis=0)
    mean_power_down_intel = np.mean(power_down_intel, axis=0)
    mean_power_up_acpi = np.mean(power_up_acpi, axis=0)
    mean_power_down_acpi = np.mean(power_down_acpi, axis=0)

    print("Up Intel: \n", mean_power_up_intel)
    print("Down Intel: \n", mean_power_down_intel)
    print("Up Acpi: \n", mean_power_up_acpi)
    print("Down Acpi: \n", mean_power_down_acpi)

    # Plot data
    legend = ["Intel", "ACPI"]
    savedir_up = base_savedir + "increase.png"
    savedir_down = base_savedir + "decrease.png"
    print(savedir_up)
    print(savedir_down)

    makePlotComb(
        mean_power_down_intel,
        mean_power_down_acpi,
        title="Frequency Decrease",
        ylabel="power consumption [W]",
        legend=legend,
        savedir=savedir_down,
    )

    makePlotComb(
        mean_power_up_intel,
        mean_power_up_acpi,
        title="Frequency Increase",
        ylabel="power consumption [W]",
        legend=legend,
        savedir=savedir_up,
    )
