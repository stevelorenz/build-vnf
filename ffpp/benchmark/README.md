# Benchmark FFPP performance #

## Setup 1: Use ComNetsEmu emulator ##

1.  Install ComNetsEmu with [install_comnetsemu.sh](../../script/install_comnetsemu.sh).
1.  Build the container image with latency benchmark (lat_bm) tools with [./build.sh](./build.sh).
1.  Run latency measurement on chain topology with `sudo python3 ./chain.py`
