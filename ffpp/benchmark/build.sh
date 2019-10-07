#!/bin/bash
#
# About: Build latency measurement container
#

docker build --compress -t lat_bm:latest -f ./Dockerfile .
