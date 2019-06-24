#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: VNF forwarding graph description parser
"""

from yaml import load, dump


def test():
    with open("./vnffgd.yaml", "r") as f:
        vnffgd = load(f)
        import ipdb
        ipdb.set_trace()


def parsef(path):
    """Parse the VNFFGD file

    :param path:
    """
    with open(path, "r") as f:
        vnffgd = load(f)


if __name__ == "__main__":
    test()
