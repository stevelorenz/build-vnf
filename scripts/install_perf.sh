#! /bin/bash
#
# About: Install perf: Linux profiling tool with performance counters
#

apt-get install -y linux-tools-common linux-tools-generic linux-tools-"$(uname -r)"
