#!/bin/bash
# About: Run some basic tests from the official documentation with loop interfaces.

cd ../ || exit
# -l: define the number of latency measurement packets per second. It is independent of the main traffic stream.
# --l-pkt-mode: Specify the type of the packets used for latency measurement.
# Check https://trex-tgn.cisco.com/trex/doc/trex_manual.html#_measuring_jitter_latency for details.
./t-rex-64 -f ./cap2/dns.yaml -m 1 -d 30 -l 1 --l-pkt-mode 0 --hdrh
