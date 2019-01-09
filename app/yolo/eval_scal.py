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

# Depends on the available cores (physical cores when Hyperthreading is disabled)
# MARK:
MAX_COTAINER_NUM = 2
IMAGE_ST = 0
IMAGE_NUM = 5


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


def mon_delay():
    """Monitor the processing delay"""
    client = docker.from_env()
    print("[MON] Monitor the processing delay")
    for i in range(MAX_COTAINER_NUM):
        print("Run %d container" % (i+1))
        for j in range(i+1):
            cmd = "./yolo_v2_pp.out -- -m 1 -s %d -n %d -p %s_%s" % (
                IMAGE_ST, IMAGE_NUM, i, j)
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
        # Wait for all running containers to exist
        while True:
            if client.containers.list():
                time.sleep(3)
            else:
                break
        print("# All running container terminated")


def mon_resource(monitor_time):
    """TODO"""
    client = docker.from_env()

    print(
        "* Start {} container(s) for YOLO pre-processing!".format(CONTAINER_NUM))
    container_lst = list()
    volumes = {
        "/vagrant/dataset": {"bind": "/dataset", "mode": "rw"},
        "/vagrant/model": {"bind": "/model", "mode": "rw"},
        "/vagrant/app": {"bind": "/app", "mode": "rw"}
    }
    for v in ["/sys/bus/pci/drivers", "/sys/kernel/mm/hugepages",
              "/sys/devices/system/node", "/dev"]:
        volumes[v] = {"bind": v, "mode": "rw"}
    container = client.containers.run(
        "darknet",
        privileged=True,
        cpuset_cpus="0",
        mem_limit="1024G",
        volumes=volumes,
        detach=True,
        working_dir="/app/yolo/",
        command=TEST_CMD
    )

    # Monitor resource usage
    print("* Start monitoring the CPU and memory usage")
    usg_lst = list()
    for i in range(monitor_time):
        try:
            st = time.time()
            stats = container.stats(decode=True, stream=False)
            mem_stats = stats["memory_stats"]
            mem_usg = mem_stats["usage"] / (1024 ** 2)
            cpu_usg = calculate_cpu_percent(stats)
            usg_lst.append((cpu_usg, mem_usg))
            dur = time.time() - st
            time.sleep(max((1.0-dur), 0))
        except KeyError as e:
            print("KeyError detected, container may ALREADY exit.")

    print("* Store monitoring results in CSV file")
    with open("./resource_usage.csv", "w+") as csvfile:
        writer = csv.writer(csvfile, delimiter=' ',
                            quotechar='|', quoting=csv.QUOTE_MINIMAL)
        for cpu, mem in usg_lst:
            writer.writerow([cpu, mem])

    print("* Stop and remove the running container")
    container.stop()
    container.remove()


if __name__ == "__main__":
    mon_delay()
