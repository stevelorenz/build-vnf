#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8
#

import argparse
import csv
import os
import sys
import time
import multiprocessing

import docker

"""
About: Evaluate the YOLOv2 pre-processing. Used to test the YOLO model splitting
       and container scheduling mechanisms.

       E.g. Scalability and parallel compute, only PROCESSING delay is simulated
       here, the end-to-end delay with packet processing SHOULD be located in
       ../../emulation/

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

IMAGE_ST = 25
IMAGE_NUM = 5

CPU_NUM = multiprocessing.cpu_count()
CPUSET_CPUS_ALL = ",".join(map(str, range(CPU_NUM)))
print("* The set for all CPUs: %s" % CPUSET_CPUS_ALL)


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

    if cpu_percent > 100:
        cpu_percent = 100

    return cpu_percent


def run_test_container(cmd, cpuset_cpus, detach=True,
                       working_dir="/app/yolo"):
    client = docker.from_env()
    c_run = client.containers.run(
        "darknet",
        privileged=True,
        cpuset_cpus=cpuset_cpus,
        mem_limit="1024m",
        volumes=VOLUMES,
        detach=detach,
        auto_remove=True,
        working_dir=working_dir,
        command=cmd
    )
    return c_run


def compile_yolov2_pp():
    run_test_container("make -j 2", CPUSET_CPUS_ALL, False)


def debug_yolov2_pp():
    run_test_container(
        "./yolo_v2_pp.out -- -m 1 -s 29 -n 1 -c./cfg/yolov2_f8.cfg -o",
        CPUSET_CPUS_ALL, False)


def prof_yolov2_pp():
    c = run_test_container("./yolo_v2_pp.out -- -m 1 -s 0 -n 10 -k",
                           CPUSET_CPUS_ALL, True)
    run = True
    try:
        while run:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Detect keyboard interrupt, stop and remove the container.")
        run = False
    finally:
        c.stop()


def run_pf_0():
    run_test_container(
        "./yolo_v2_pp.out -- -m 0 -s %d -n %d" % (
            IMAGE_ST, IMAGE_NUM), "0", False
    )


def run_pf_1(max_ct_num, cpu_set, sched_algo, round_num):
    """
    MARK: This profile is IMPORTANT, the scheduling of multiple containers
    over a set of CPUs is a optimization problem e.g. put on the same core ->
    share the L1 cache, put on the different core -> more processing power
    """

    for i in range(max_ct_num):
        for r in range(round_num):
            print("Run %d container" % (i+1))
            print("Current test round: %d" % (r+1))
            for j in range(i+1):
                cmd = "./yolo_v2_pp.out -- -m 1 -s %d -n %d -p %s_%s_%s " % (
                    IMAGE_ST, IMAGE_NUM, (i+1), (j+1), (r+1))
                cmd = cmd + "-c ./cfg/yolov2_f8.cfg"

                if sched_algo == "round_robin":
                    cpu_set_ct = str((j % len(cpu_set)) + cpu_set[0])
                elif sched_algo == "let_kernel_do_it":
                    cpu_set_ct = ",".join(map(str, cpu_set[:]))
                else:
                    raise RuntimeError("Unknown scheduling algorithm!")

                print("Ct_index:%d, Core_idx:%s" % (j, cpu_set_ct))
                run_test_container(cmd, cpu_set_ct, True)

            # Wait for all running containers to exist
            client = docker.from_env()
            while True:
                if client.containers.list():
                    time.sleep(3)
                else:
                    break
            print("# All running container terminated")
            time.sleep(3)


def run_pf_2():
    """TODO"""
    pass


def eval_yolov2_pp(profile=0):
    """Evaluate the YOLOv2 preprocessing with different profiles
    """
    cmd_suffix = ""
    print("[EVAL] Current profile %d" % profile)

    if profile == 0:
        print("""[EVAL] Monitor the processing delay of each layer in the YOLOv2
              and YOLOv2 tiny model. Run only one container""")
        run_pf_0()
        return

    elif profile == 1:
        print("""[EVAL] Monitor the processing delay of YOLOv2 pre-processing.
              The scalability is simulated for different number of CPUs and
              different scheduling algorithms""")
        # TODO: Extend the simulation to more scheduling
        run_pf_1(2, [0, 1], "round_robin", 1)
        time.sleep(5)

    elif profile == 2:
        run_pf_2()
        # max_cotainer_num = 1
        # test_mode = 1
        # cmd_suffix = "-k"
        # cpu_set = "0"
        # num_run = 1

    # for i in range(max_cotainer_num):
    #    for r in range(num_run):
    #        print("Run %d container" % (i+1))
    #        print("Current test round: %d" % (r+1))
    #        started = list()
    #        for j in range(i+1):
    #            cmd = "./yolo_v2_pp.out -- -m %d -s %d -n %d -p %s_%s_%s " % (
    #                test_mode, IMAGE_ST, IMAGE_NUM, (i+1), (j+1), (r+1))
    #            cmd = cmd + cmd_suffix
    #            if profile == 1 and i == 1:
    #                cpu_set = str(j)
    #                # print(cpu_set)
    #            c = client.containers.run(
    #                "darknet",
    #                privileged=True,
    #                cpuset_cpus=cpu_set,
    #                mem_limit="1024m",  # TODO: Reduce memory limit
    #                volumes=VOLUMES,
    #                detach=True,
    #                working_dir="/app/yolo/",
    #                command=cmd
    #            )
    #            started.append(c)

    #            if profile == 2:
    #                print("Start resource monitoring")
    #                usg_lst = list()
    #                for i in range(30):
    #                    try:
    #                        st = time.time()
    #                        stats = started[0].stats(decode=True, stream=False)
    #                        mem_stats = stats["memory_stats"]
    #                        mem_usg = mem_stats["usage"] / (1024 ** 2)
    #                        cpu_usg = calculate_cpu_percent(stats)
    #                        usg_lst.append((cpu_usg, mem_usg))
    #                        dur = time.time() - st
    #                        time.sleep(max((1.0-dur), 0))
    #                    except KeyError:
    #                        print("KeyError detected, container may ALREADY exit.")

    #                print("* Store monitoring results in CSV file")
    #                with open("./resource_usage.csv", "w+") as csvfile:
    #                    writer = csv.writer(csvfile, delimiter=' ',
    #                                        quotechar='|', quoting=csv.QUOTE_MINIMAL)
    #                    for cpu, mem in usg_lst:
    #                        writer.writerow([cpu, mem])
    #                for c in started:
    #                    c.stop()

    #        # Wait for all running containers to exist
    #        while True:
    #            if client.containers.list():
    #                time.sleep(3)
    #            else:
    #                break
    #        print("# All running container terminated")
    #        for c in started:
    #            c.remove()

    #    time.sleep(5)


def print_help():
    usage = """
Usage: sudo python3 ./eval_proc.py OPTION
       Check the main and eval_yolov2_pp function in the source to get
       information about OPTION.
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
            eval_yolov2_pp(profile)
