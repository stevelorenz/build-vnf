#!/bin/bash
#
# About: Setup DPDK on a test VM without binding any interfaces.
# Email: xianglinks@gmail.com
# Ref  : Official dpdk-setup.sh

###############
#  Variables  #
###############

# DPDK version to use. If unspecified use master branch
export DPDK_VERSION=v17.05

# Path to the DPDK dir
export RTE_SDK=${HOME}/dpdk

# Target of build process
export RTE_TARGET=x86_64-native-linuxapp-gcc

# Hugepages mount point
export HUGEPAGE_MOUNT=/mnt/huge

###################
#  Setup Process  #
###################

# Configure hugepages
# MARK: The allocation of hugepages SHOULD be done at boot time or as soon as possible after system boot
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
sudo mkdir ${HUGEPAGE_MOUNT}
sudo mount -t hugetlbfs nodev ${HUGEPAGE_MOUNT}

# Set Hugepages in /etc/fstab so they persist across reboots
echo "nodev ${HUGEPAGE_MOUNT} hugetlbfs defaults 0 0" | sudo tee -a /etc/fstab

# Install dependencies
echo "# Installing dependencies via apt."
sudo apt-get update
sudo apt-get -y install git hugepages build-essential clang doxygen
sudo apt-get -y install libnuma-dev libpcap-dev linux-headers-`uname -r`

# Config and build DPDK
# get code from git repo
git clone http://dpdk.org/git/dpdk ${RTE_SDK}
cd ${RTE_SDK}
make config T=${RTE_TARGET}
sed -ri 's,(PMD_PCAP=).*,\1y,' build/.config
echo "# Compiling dpdk target from source."
make

# Install and load kernel modules
echo "# Loading kernel modules."
sudo modprobe uio
sudo insmod ${RTE_SDK}/build/kmod/igb_uio.ko

# Make uio and igb_uio persist across reboots
sudo ln -sf ${RTE_SDK}/build/kmod/igb_uio.ko /lib/modules/`uname -r`
sudo depmod -a
echo "uio" | sudo tee -a /etc/modules
echo "igb_uio" | sudo tee -a /etc/modules

# Add env variables setting to .profile file so that they are set at each login
echo "export RTE_SDK=${RTE_SDK}" >> ${HOME}/.profile
echo "export RTE_TARGET=${RTE_TARGET}" >> ${HOME}/.profile

# Link RTE_TARGET to already built binaries, needed by default make files
echo "# Link built binaries."
ln -sf ${RTE_SDK}/build ${RTE_SDK}/${RTE_TARGET}

echo ""
echo "# Setup finished."
