"""
About: Script to plot the power consumption boundaries
and temperature of the tested CPU.
"""

import sys
import os
import numpy as np
import matplotlib.pyplot as plt

freq2idx = {
    '800-MHz': 0,
    '3400-MHz': 1,
    '4000-MHz': 2
}


def getData(path, power, temp, pos, mask) :
    plot_name = None
    for file in os.listdir(path):
        filename = os.fsdecode(file)
        file = path + filename

        if file.endswith('ts'):
            if plot_name is None:
                plot_name = filename.split('_')[1:3]
                plot_name = (plot_name[0] + '_' + plot_name[1]).split('.')[0]
            m_idx = freq2idx[filename.split('_')[0]]
            data = np.genfromtxt(file, delimiter=',')[2::mask]
            power[m_idx] = data[:,4].copy()
            temp[m_idx] = data[:,2].copy()

    return plot_name

def getStats(data) :
    m_stats = []

    m_stats.append(np.mean(data, axis=1))
    m_stats.append(np.std(data, axis=1))

    return m_stats

def getLegend(raw_legend, stats) :
    m_label = []

    for m_idx in range (0, stats[0].shape[0], 1):
        m_label.append(raw_legend[m_idx] + '; µ: ' +
                        str(round(stats[0][m_idx], 3)) +
                        ', std: ' +
                        str(round(stats[1][m_idx], 4)))

    return m_label

def makePlot(data, title=None, ylabel=None, legend=None, savedir=None) :
    stats = getStats(data)
    new_legend = None
    if legend :
        new_legend = getLegend(legend, stats)
    for m_idx in range(0, data.shape[0], 1):
        plt.plot(data[m_idx])

    plt.grid()
    plt.title(title)
    plt.ylabel(ylabel)
    plt.xlabel('Iteration')
    plt.legend((new_legend))
    if savedir:
        savedir += '_' + ylabel.split(' ')[0] + '.png'
        print(savedir)
        plt.savefig(savedir)
        plt.close()
    else:
        plt.show()


if __name__ == '__main__' :
    path = './turbostat_stats/30/'
    savedir = './turbostat_stats/'
    mask = 3
    pos = 4
    num_it = 10
    frequencies = 3
    power = np.zeros((frequencies, num_it))
    temperature = np.zeros((frequencies, num_it))
    raw_legend = ['800 MHz', '3400 MHz', '4000MHz']

    plot_name = getData(path, power, temperature, pos, mask)
    savedir += plot_name

    makePlot(power, title='Power Consumption',
            ylabel='power consumption [W]', legend=raw_legend,
            savedir=savedir)
    makePlot(temperature, title='CPU Package Temperature',
            ylabel='temperature [°C]', legend=raw_legend,
            savedir=savedir)

