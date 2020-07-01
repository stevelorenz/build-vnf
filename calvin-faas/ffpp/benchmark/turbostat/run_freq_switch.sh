#! /bin/bash
#
# About: Script to run the frequency_switch programm in the
# power-manager container. 
# Extra script to put process reliably into the background.

DOCKER_CMD="taskset 4 ./user/build/examples/ffpp_frequency_switcher -s 1000000 -t -l 2"

taskset 4 sudo docker exec -d power-manager /bin/sh -c "${DOCKER_CMD}"
