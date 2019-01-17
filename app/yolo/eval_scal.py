#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

import argparse
import csv
import sys
import time

import docker

"""
About: Evaluate the YOLOv2 pre-processing. E.g. Scalability and parallel compute

Required Configurations:
    1. Allocate hugepage to run DPDK, check ../../script/setup_hugepage.sh

Email: xianglinks@gmail.com
"""


VOLUMES = {
    "/vagrant/dataset": {"bind": "/dataset", "mode": "rw"},
    "/vagrant/app": {"bind": "/app", "mode": "rw"}
}
# Required for runnning DPDK inside container
for v in ["/sys/bus/pci/drivers", "/sys/kernel/mm/hugepages",
          "/sys/devices/system/node", "/dev"]:
    VOLUMES[v] = {"bind": v, "mode": "rw"}

IMAGE_ST = 0
IMAGE_NUM = 5
MAX_CONTAINER_NUM = 1


def calculate_cpu_percent(d):
    """ISSUE: Report the CPU usage > 100%
    """
    cpu_count = len(d["cpu_stats"]["cpu_usage"]["percpu_usage"])
    cpu_percent = 0.0
    cpu_delta = float(d["cpu_stats"]["cpu_usage"]["total_usage"]) - \
        float(d["precpu_stats"]["cpu_usage"]["total_usage"])
    system_delta = float(d["cpu_stats"]["system_cpu_usage"]) - \
        float(d["precpu_stats"]["system_cpu_usage"])
    if system_delta > 0.0:
        cpu_percent = cpu_delta / system_delta * 100.0 * cpu_count

    return cpu_percent


def compile_yolov2_pp():
    client = docker.from_env()
    client.containers.run(
        "darknet",
        privileged=True,
        mem_limit="1024m",
        volumes=VOLUMES,
        detach=False,
        working_dir="/app/yolo/",
        command="make -j 2"
    )
    for c in client.containers.list(all=True):
        c.remove()


def debug_yolov2_pp():
    client = docker.from_env()
    c = client.containers.run(
        "darknet",
        privileged=True,
        cpuset_cpus="0",
        mem_limit="1024m",
        volumes=VOLUMES,
        detach=False,
        working_dir="/app/yolo/",
        command="./yolo_v2_pp.out -- -m 1 -s 29 -n 1 -c ./cfg/yolov2_f8.cfg -o"
    )
    for c in client.containers.list(all=True):
        c.remove()


def prof_yolov2_pp():
    client = docker.from_env()
    c = client.containers.run(
        "darknet",
        privileged=True,
        cpuset_cpus="0",
        mem_limit="1024m",
        volumes=VOLUMES,
        detach=True,
        working_dir="/app/yolo/",
        command="./yolo_v2_pp.out -- -m 1 -s 0 -n 10 -k"
    )
    run = True
    try:
        while run:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Detect keyboard interrupt, stop and remove the container.")
        run = False
    finally:
        c.stop()
        c.remove()


def mon_yolov2_pp(profile=0):
    cmd_suffix = ""
    print("[MON] Current profile %d" % profile)
    if profile == 0:
        # Depends on the available cores (physical cores when Hyperthreading is disabled)
        max_cotainer_num = 1
        test_mode = 0
        cpu_set = "0"
        num_run = 1
    elif profile == 1:
        # MARK: increase this number until the pre-processing can not be
        max_cotainer_num = MAX_CONTAINER_NUM
        test_mode = 1
        cpu_set = "0,1"
        num_run = 1
        cmd_suffix = "-c ./cfg/yolov2_f4.cfg"
    elif profile == 2:
        max_cotainer_num = 1
        test_mode = 1
        cmd_suffix = "-k"
        cpu_set = "0"
        num_run = 1

    # TODO:  <14-01-19, zuo> Add a profile to measure the batching effects #

    client = docker.from_env()
    if profile == 0 or profile == 1:
        print("[MON] Monitor the processing delay")

    for i in range(max_cotainer_num):
        for r in range(num_run):
            print("Run %d container" % (i+1))
            print("Current test round: %d" % (r+1))
            started = list()
            for j in range(i+1):
                cmd = "./yolo_v2_pp.out -- -m %d -s %d -n %d -p %s_%s_%s " % (
                    test_mode, IMAGE_ST, IMAGE_NUM, (i+1), (j+1), (r+1))
                cmd = cmd + cmd_suffix
                if profile == 1 and i == 1:
                    cpu_set = str(j)
                    # print(cpu_set)
                c = client.containers.run(
                    "darknet",
                    privileged=True,
                    cpuset_cpus=cpu_set,
                    mem_limit="1024m",  # TODO: Reduce memory limit
                    volumes=VOLUMES,
                    detach=True,
                    working_dir="/app/yolo/",
                    command=cmd
                )
                started.append(c)

                if profile == 2:
                    print("Start resource monitoring")
                    usg_lst = list()
                    for i in range(30):
                        try:
                            st = time.time()
                            stats = started[0].stats(decode=True, stream=False)
                            mem_stats = stats["memory_stats"]
                            mem_usg = mem_stats["usage"] / (1024 ** 2)
                            cpu_usg = calculate_cpu_percent(stats)
                            usg_lst.append((cpu_usg, mem_usg))
                            dur = time.time() - st
                            time.sleep(max((1.0-dur), 0))
                        except KeyError:
                            print("KeyError detected, container may ALREADY exit.")

                    print("* Store monitoring results in CSV file")
                    with open("./resource_usage.csv", "w+") as csvfile:
                        writer = csv.writer(csvfile, delimiter=' ',
                                            quotechar='|', quoting=csv.QUOTE_MINIMAL)
                        for cpu, mem in usg_lst:
                            writer.writerow([cpu, mem])
                    for c in started:
                        c.stop()

            # Wait for all running containers to exist
            while True:
                if client.containers.list():
                    time.sleep(3)
                else:
                    break
            print("# All running container terminated")
            for c in started:
                c.remove()

        time.sleep(5)


def print_help():
    usage = """
Usage: sudo python3 ./eval_scal.py OPTION

- OPTION:
    -p : Run profiling (only used for development).
    0: Run evaluation measurements for per-layer latency of YOLOv2 and
    YOLOv2-tiny.
    1: Run evaluation measurements for YOLOv2 pre-processing
    """
    print(usage)


if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Missing options!")
        print_help()
    else:
        if sys.argv[1] == "-p":
            print("* Profiling the YOLOv2 pre-processing...")
            compile_yolov2_pp()
            prof_yolov2_pp()
        elif sys.argv[1] == "-d":
            print("* Debug YOLOv2 pre-processing")
            compile_yolov2_pp()
            debug_yolov2_pp()
        else:
            profile = int(sys.argv[1])
            if profile >= 2:
                print("Invalid evaluation profile!")
                print_help()
                sys.exit(1)
            print("* Running evaluation measurements with profile %d..." %
                  profile)
            compile_yolov2_pp()
            mon_yolov2_pp(profile)
