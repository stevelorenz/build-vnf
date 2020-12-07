#!/bin/bash
#
# About: Setup BPF Compiler Collection (BCC)
#

# BCC is now available in official repository of Debian and Ubuntu
apt-get install bpfcc-tools linux-headers-$(uname -r) libbpfcc-dev python3-bpfcc
