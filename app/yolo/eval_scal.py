#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

import csv
import time
import sys

import docker

"""
About: Evaluate the YOLOv2 pre-processing. E.g. Scalability and parallel compute
Email: xianglinks@gmail.com
"""


# MONITOR_TIME = 2 * IMAGE_NUM  # seconds
# CONTAINER_NUM = 1  # TODO: Use two containers, with CPU set 1, 2


VOLUMES = {
    "/vagrant/dataset": {"bind": "/dataset", "mode": "rw"},
    "/vagrant/model": {"bind": "/model", "mode": "rw"},
    "/vagrant/app": {"bind": "/app", "mode": "rw"}
}
for v in ["/sys/bus/pci/drivers", "/sys/kernel/mm/hugepages",
          "/sys/devices/system/node", "/dev"]:
    VOLUMES[v] = {"bind": v, "mode": "rw"}

IMAGE_ST = 0
IMAGE_NUM = 5


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


def mon_yolov2_pp(profile=0):
    print("[MON] Current profile %d" % profile)
    cmd_suffix = ""
    if profile == 0:
        # Depends on the available cores (physical cores when Hyperthreading is disabled)
        max_cotainer_num = 1
        test_mode = 0
    elif profile == 1:
        max_cotainer_num = 2
        test_mode = 1
    elif profile == 2:
        max_cotainer_num = 1
        test_mode = 1
        cmd_suffix = "-k"

    client = docker.from_env()
    if profile == 0 or profile == 1:
        print("[MON] Monitor the processing delay")
    for i in range(max_cotainer_num):
        started = list()
        print("Run %d container" % (i+1))
        for j in range(i+1):
            cmd = "./yolo_v2_pp.out -- -m %d -s %d -n %d -p %s_%s " % (
                test_mode, IMAGE_ST, IMAGE_NUM, i, j)
            cmd = cmd + cmd_suffix
            c = client.containers.run(
                "darknet",
                privileged=True,
                cpuset_cpus=str(j),
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


if __name__ == "__main__":
    compile_yolov2_pp()
    for p in range(3):
        mon_yolov2_pp(p)
