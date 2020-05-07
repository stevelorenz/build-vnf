#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

import csv
import time

import docker

"""
About: Perf the resource usage of the docker container running the edge boxes
       detection. It is based-on docker-py and docker client implementation.
"""

TEST_CMD = "python ./rpa.py -l"  # Run RPA in loop mode
MONITOR_TIME = 5  # seconds
CONTAINER_NUM = 1  # For scalability


def calculate_cpu_percent(d):
    cpu_count = len(d["cpu_stats"]["cpu_usage"]["percpu_usage"])
    cpu_percent = 0.0
    cpu_delta = float(d["cpu_stats"]["cpu_usage"]["total_usage"]) - float(
        d["precpu_stats"]["cpu_usage"]["total_usage"]
    )
    system_delta = float(d["cpu_stats"]["system_cpu_usage"]) - float(
        d["precpu_stats"]["system_cpu_usage"]
    )
    if system_delta > 0.0:
        cpu_percent = cpu_delta / system_delta * 100.0 * cpu_count

    return cpu_percent


def main(monitor_time):
    client = docker.from_env()

    print("* Start {} container(s) for edge boxes detection".format(CONTAINER_NUM))
    container = client.containers.run(
        "jjanzic/docker-python3-opencv:contrib-opencv-3.4.2",
        cpuset_cpus="1",
        mem_limit="1024G",
        volumes={
            "/vagrant/dataset": {"bind": "/dataset", "mode": "ro"},
            "/vagrant/model": {"bind": "/model", "mode": "ro"},
            "/vagrant/app": {"bind": "/app", "mode": "rw"},
        },
        detach=True,
        working_dir="/app/distributed_rcnn/",
        command=TEST_CMD,
    )

    # Monitor resource usage
    time.sleep(5)
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
            time.sleep(max((1.0 - dur), 0))
        except KeyError:
            print("KeyError detected, container may ALREADY exit.")

    print("* Store monitoring results in CSV file")
    with open("./resource_usage.csv", "w+") as csvfile:
        writer = csv.writer(
            csvfile, delimiter=" ", quotechar="|", quoting=csv.QUOTE_MINIMAL
        )
        for cpu, mem in usg_lst:
            writer.writerow([cpu, mem])

    print("* Stop and remove the running container")
    container.stop()
    container.remove()


if __name__ == "__main__":
    main(MONITOR_TIME)
