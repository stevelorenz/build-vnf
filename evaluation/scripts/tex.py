import matplotlib
from math import sqrt


def setup(width=1, *, height=None, span=False, l=0.15, r=0.98, t=0.98, b=0.17, params={}):
    if span:
        fig_width = 529.22128 / 72  # IEEE text width
    else:
        fig_width = 258.61064 / 72  # IEEE column width

    if not height:
        golden_mean = (sqrt(5)-1.0)/2.0    # Aesthetic ratio
        fig_height = (258.61064 / 72)*golden_mean  # height in inches
    else:
        fig_height = height

    fig_width = fig_width * width

    # see http://matplotlib.org/users/customizing.html for more options
    rc = {'backend': 'ps',
          'text.usetex': True,
          'text.latex.preamble': ['\\usepackage{gensymb}'],
          'axes.labelsize': 8,  # fontsize for x and y labels (was 10)
          'axes.titlesize': 8,
          'font.size': 8,  # was 10
          'legend.fontsize': 8,  # was 10
          'xtick.labelsize': 8,
          'ytick.labelsize': 8,
          'figure.figsize': [fig_width, fig_height],
          'font.family': 'serif',
          'figure.subplot.left': l,
          'figure.subplot.right': r,
          'figure.subplot.bottom': b,
          'figure.subplot.top': t,
          'savefig.dpi': 300
          }
    rc.update(params)

    matplotlib.rcParams.update(rc)


def save_fig(fig, path, fmt="pdf"):
    """Save fig to path with given format"""
    fig.savefig(path + '.%s' % fmt, pad_iches=0,
                bbox_inches='tight', dpi=1200, format=fmt)
