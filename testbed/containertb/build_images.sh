#! /bin/bash
#
# About: Build docker images for DPDK tests
#

sudo docker build -t dpdk_base:v0.1 -f ./Dockerfile.dpdk_base .
sudo docker build -t dpdk_pktgen:v0.1 -f ./Dockerfile.dpdk_pktgen .
