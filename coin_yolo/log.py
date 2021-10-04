#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: log
"""

import logging


LEVELS = {
    "debug": logging.DEBUG,
    "info": logging.INFO,
}


def get_logger(level):
    """get_logger"""
    log_level = LEVELS.get(level, None)
    if not log_level:
        raise RuntimeError("Unknown logging level!")
    logger = logging.getLogger("meica_host")
    handler = logging.StreamHandler()
    formatter = logging.Formatter("%(asctime)s %(levelname)-6s %(message)s")
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(log_level)
    return logger
