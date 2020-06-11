#!/bin/bash

# for i in {2..3}; do
   # echo "Disabling logical HT core $i."
   # echo 0 > /sys/devices/system/cpu/cpu${i}/online;
# done

if [[ $# -ne 1 ]]; then
    echo 'Require argument. 0 to turn off hyper-threading, 1 to turn it on'
    exit 1
fi

# Check with $(cat /proc/cpuinfo) which cores share the same core_id
# and add/modifiy respecive lines
echo $1 > /sys/devices/system/cpu/cpu1/online
echo $1 > /sys/devices/system/cpu/cpu3/online

grep "" /sys/devices/system/cpu/cpu*/topology/core_id
