#!/bin/bash
#
# install_energy_meter.sh
#

cd "$HOME" || exit
echo "* Install Intel CPU energy meter from sosy-lab LUM."
sudo apt install -y libcap-dev
sudo modprobe msr
sudo modprobe cpuid
git clone https://github.com/sosy-lab/cpu-energy-meter
cd cpu-energy-meter && make
