#! /bin/bash
#
# About: Measure the CPU power consumption in all possible p-states
# while a stress-ng test is running to make CPU busy.
# All tasks are distributed on different cores to avoid interference.
#
# ToDo: Kill stress-ng automatically in the end
#

COUNTER=0

HOME="/home/malte/"
DIR="build-vnf/calvin-faas/ffpp/util/"

cd $DIR
sudo ./setup_hugepage.sh
sudo ./power_manager stop
taskset 4 sudo ./power_manager.py run
cd $HOME

taskset 1 stress-ng --matrix 1 -t 200m &

echo "Start test!"
while [ $COUNTER -le 99 ]; do
    sudo taskset 4 ./run_freq_switch.sh &
    sleep 1
    sudo taskset 2 ./turbostat.sh -n 4000 -t 0.001 -f switch-acpi-$COUNTER
    ((COUNTER++))
done

echo "Done with test, you may abort stress-ng now."
