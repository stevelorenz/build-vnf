#!/bin/bash
#
# About: Install pktgen traffic generator

PKTGEN_ROOT="${HOME}/pktgen-dpdk"

# Clone and build source code
git clone git://dpdk.org/apps/pktgen-dpdk "${HOME}/pktgen-dpdk"
cd ${PKTGEN_ROOT}
git checkout -b pktgen-3.5.0
make
