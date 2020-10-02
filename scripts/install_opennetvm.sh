#! /bin/bash
#
# About: Install OpenNetVM
#

sudo apt update
sudo apt-get install -y build-essential linux-headers-$(uname -r) git libnuma-dev

cd $HOME || exit
git clone https://github.com/sdnfv/openNetVM

# Use default develop branch
cd openNetVM || exit
#git checkout master
# Init DPDK sub-module
git submodule sync
git submodule update --init
export ONVM_HOME
ONVM_HOME=$(pwd)
echo export ONVM_HOME=$(pwd) >>~/.bashrc
cd dpdk || exit
export RTE_SDK
RTE_SDK=$(pwd)
export RTE_TARGET=x86_64-native-linuxapp-gcc
export ONVM_NUM_HUGEPAGES=512

# Disable ASLR since it makes sharing memory with NFs harder:
sudo sh -c "echo 0 > /proc/sys/kernel/randomize_va_space"

cd $HOME/openNetVM/scripts || exit
bash ./install.sh

cd $HOME/openNetVM/onvm || exit
make -j $(nproc)
cd ../examples
make -j $(nproc)
