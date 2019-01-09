#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

"""
About:
"""


def calculate_cpu_percent(d):
    cpu_count = len(d["cpu_stats"]["cpu_usage"]["percpu_usage"])
    cpu_percent = 0.0
    cpu_delta = float(d["cpu_stats"]["cpu_usage"]["total_usage"]) - \
        float(d["precpu_stats"]["cpu_usage"]["total_usage"])
    system_delta = float(d["cpu_stats"]["system_cpu_usage"]) - \
        float(d["precpu_stats"]["system_cpu_usage"])
    if system_delta > 0.0:
        cpu_percent = cpu_delta / system_delta * 100.0 * cpu_count

    return cpu_percent
