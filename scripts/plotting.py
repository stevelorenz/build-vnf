"""
Some little helpers for plotting figures in IEEE papers
"""

import os
import sys
import argparse
import matplotlib
import matplotlib.pyplot as plt
from math import sqrt

plot_files = []


def save(figdir, path):
    abspath = os.path.join(figdir, path)
    print(abspath)
    plt.savefig(abspath, bbox_inches="tight")
    plot_files.append(abspath)


def get_figsize(width, height, span):
    if span:
        fig_width = 529.22128 / 72  # IEEE text width
    else:
        fig_width = 258.61064 / 72  # IEEE column width
    if not height:
        golden_mean = (sqrt(5) - 1.0) / 2.0  # Aesthetic ratio
        fig_height = (258.61064 / 72) * golden_mean  # height in inches
        fig_height = fig_height * 0.9
    else:
        fig_height = height
    fig_width = fig_width * width
    return fig_width, fig_height


def setup(
    width=1, *, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17, params={}
):
    figsize = get_figsize(width, height, span)
    # see http://matplotlib.org/users/customizing.html for more options
    rc = {
        "backend": "ps",
        # "text.usetex": True,
        # "text.latex.preamble": ["\\usepackage{gensymb}"],
        "axes.labelsize": 10,
        "axes.titlesize": 10,
        "font.size": 10,
        "legend.fontsize": 10,
        "xtick.labelsize": 10,
        "ytick.labelsize": 10,
        "figure.figsize": figsize,
        "font.family": "serif",
        "figure.subplot.left": l,
        "figure.subplot.right": r,
        "figure.subplot.bottom": b,
        "figure.subplot.top": t,
        "savefig.dpi": 300,
        "lines.linewidth": 1,
        "errorbar.capsize": 2,
    }
    rc.update(params)
    matplotlib.rcParams.update(rc)


def legend(lines, width=1, height=None, span=False, **kwargs):
    figsize = get_figsize(width, height, span)
    fig = plt.figure(figsize=figsize)
    fig.legend(tuple(lines), tuple([l.get_label() for l in lines]), "center", **kwargs)
    return fig


def main(query, plot):
    here = os.path.dirname(sys.argv[0])
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--show", help="Show the plots instead of saving to file", action="store_true"
    )
    parser.add_argument(
        "--query", help="Show the output of the query", action="store_true"
    )
    parser.add_argument(
        "--input", help="Input directory", default=os.path.join(here, "../data")
    )
    parser.add_argument(
        "--output", help="Output directory", default=os.path.join(here, "../figures")
    )
    args = parser.parse_args()
    os.system("mkdir -p '%s'" % args.output)
    df = query(args.input)
    if args.query:
        print(df)
    plot(df, args.output)
    if args.show:
        for fn in plot_files:
            os.system("xdg-open >/dev/null 2>/dev/null '%s'" % fn)
