#! /usr/bin/env python
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: Monitor CPU and Memory Usage
       Used for resource monitoring of different VNF implementations
"""

import sched
import time
import sys

import psutil

GET_CPU_PERIOD_S = 0.1
LOG_CPU_PERIOD_S = 1.0
CPU_LOG_FILE = './cpu_usage.csv'
CPU_USAGE = list()


def get_cpu_usage(scheduler):
    print(psutil.cpu_percent(interval=0, percpu=True))
    scheduler.enter(GET_CPU_PERIOD_S, 1, get_cpu_usage,
                    argument=(scheduler, ))


def log_cpu_usage(scheduler):
    # Log CPU usage, SHOULD be fast
    print("Write into file...")
    print(CPU_USAGE)
    # Add the task in the queue
    scheduler.enter(LOG_CPU_PERIOD_S, 2, log_cpu_usage,
                    argument=(scheduler, ))


def main():
    scheduler = sched.scheduler(timefunc=time.time, delayfunc=time.sleep)
    # Add tasks in the scheduler
    scheduler.enter(GET_CPU_PERIOD_S, 1, get_cpu_usage,
                    argument=(scheduler, ))
    scheduler.enter(LOG_CPU_PERIOD_S, 2, log_cpu_usage,
                    argument=(scheduler, ))

    try:
        scheduler.run(blocking=True)
    except KeyboardInterrupt:
        print("KeyboardInterrupt detected, exit the program...")
        sys.exit(0)


if __name__ == '__main__':
    main()
