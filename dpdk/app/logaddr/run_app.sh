#!/bin/bash
# About: An example of running the forwarding APP
# Param: Use the first lcore for both rx and tx ports

sudo ./build/logaddr -l 0 -- -p 0x3 -q 2
