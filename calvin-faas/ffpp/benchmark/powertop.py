#! /usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:fenc=utf-8

"""
About: A simple wrapper for powertop for CPU power management.
"""

import argparse
import csv
import os
import re
import shlex
import subprocess
import sys
import tempfile

from time import sleep


class Powertop(object):
    """Powertop"""

    def __init__(self, command="/usr/sbin/powertop -q"):
        self.command = command
        self.pid = -1
        self.csv_name = ""

    def get_results(self, pid, time, iteration, pause):
        print("*** The PID of the process to measure is : {}".format(pid))
        results = [pid]
        print(
            "*** Total iteration number: {}, pause: {} seconds".format(iteration, pause)
        )
        for i in range(iteration):
            print("- Current iteration number: {}".format(i + 1))
            self.pid = pid
            csv_data = self._run(time)
            results.append(self._parse_csv(csv_data))
            sleep(pause)
        return results

    def _run(self, time):
        with tempfile.NamedTemporaryFile(mode="w+", suffix=".csv", delete=True) as fd:
            pt_opts = " ".join(
                (
                    "--csv={}".format(fd.name),
                    "--time={}".format(time),
                )
            )
            cmd = " ".join((self.command, pt_opts))
            subprocess.run(shlex.split(cmd))
            return fd.readlines()

    def _parse_csv(self, csv_data):
        power_consumer_section = self._get_power_consumer_section(csv_data)
        line_wanted = ""
        for line in power_consumer_section:
            search_obj = re.search(r"\[PID (\d+)\]", line)
            if search_obj:
                if self.pid == int(search_obj.group(1)):
                    line_wanted = line.strip()
                    break
        else:
            print(
                "Can not find the power information of the PID: {}".format(self.pid),
                file=sys.stderr,
            )
        esti_power = line_wanted.split(";")[-1].strip()
        return esti_power

    def _get_power_consumer_section(self, csv_data):
        start_index = 0
        end_index = 0
        found_start = False
        for idx, line in enumerate(csv_data):
            line = line.strip()
            if "Overview of Software Power Consumers" in line:
                start_index = idx
                found_start = True
            if found_start and set(line) == {"_"}:
                end_index = idx
                break
        assert end_index >= start_index

        return csv_data[start_index:end_index]

    def save_csv(self, results):
        if not results:
            print("ERR: Results are empty!", sys.stderr)
            return
        csv_name = "powertop_results.csv"
        self.csv_name = csv_name
        with open(csv_name, "a+") as csvfile:
            writer = csv.writer(csvfile, delimiter=",")
            writer.writerow(results)

    @staticmethod
    def check_pid(pid):
        try:
            os.kill(pid, 0)
        except OSError:
            return False
        else:
            return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="A simple wrapper for powertop for CPU power management."
    )
    parser.add_argument("pid", type=int, help="The PID of the process to measure.")
    parser.add_argument("time", type=int, help="The number of seconds to measure.")
    parser.add_argument(
        "--iteration",
        type=int,
        default=1,
        help="Number of times to run each measurement.",
    )
    parser.add_argument(
        "--pause",
        type=int,
        default=3,
        help="Seconds to sleep during each iteration.",
    )
    args = parser.parse_args()
    powertop = Powertop()
    ret = powertop.check_pid(args.pid)
    if not ret:
        print("Can not find the process with PID: {}".format(args.pid), file=sys.stderr)
    else:
        ret = powertop.get_results(args.pid, args.time, args.iteration, args.pause)
        print("*** Measurement results: ")
        print(ret)
        print("- The results are saved in the CSV file: {}".format(powertop.csv_name))
        powertop.save_csv(ret)
