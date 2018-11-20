#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About: Network Service Header (RFC8300) class used for SCL and VNF
       implementation

Ref  : https://github.com/torvalds/linux/blob/master/include/net/nsh.h
       https://github.com/opendaylight/sfc/tree/master/sfc-py

Email: xianglinks@gmail.com
"""

from collections import namedtuple
from ctypes import Structure
from struct import pack

NSH_ETH_TYPE = int("0x894F", 16)
NSH_VERSION_1 = 0


class BaseHeader(object):
    """NSH base header

    - O bit: This bit indicates an Operation, Adminitration and Maintainace
      (OAM) packet.

    """

    def __init__(self, Obit, version=NSH_VERSION_1):
        pass


class ServicePathHeader(object):
    """Service Path Header
    """

    def __init__(self):
        pass


class NSH(object):
    """

    Network Service Header Format:

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                Base Header                                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                Service Path Header                            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    ~                Context Header(s)                              ~
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    """

    def __init__(self):
        pass

    def pack(self):
        pass
