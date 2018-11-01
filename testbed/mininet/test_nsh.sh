#! /bin/bash
#
# About: Test Open vSwitch NSH support
#        It uses the Mininet topo defined in ../../vnf_debug/pedestrian_detection/emulation/topo.py
#        The example chain: cam1 -> dec1 -> server
#


SW="s1"

sudo ovs-vsctl show
