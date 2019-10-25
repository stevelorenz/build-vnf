#! /bin/bash
#
# About: Run simple test with netem

SERVER_IP="10.0.0.13"
CLIENT_IP="10.0.0.15"

if [[ ${1} == "-s" ]]; then
    echo "Run iperf TCP server."
    iperf3 -s -B $SERVER_IP

elif [[ ${1} == "-c" ]]; then
    echo "Run iperf TCP client."
    sudo modprobe sch_netem
    lsmod | grep sch_netem
    sudo tc qdisc change dev ${2} root netem loss ${3}%
    sudo tc qdisc
    iperf3 -B $CLIENT_IP -c $SERVER_IP -t 10 -i 3
fi
