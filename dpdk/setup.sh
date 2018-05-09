#!/bin/bash
#
# this script downloads, installs and configure the intel dpdk framework
# on a clean ubuntu 16.04 installation running in a virtual machine.
#
# this script has been created based on the following scripts:
#  * https://gist.github.com/conradirwin/9077440
#  * http://dpdk.org/doc/quick-start
#  * https://github.com/lorenzosaino/ubuntu-dpdk/blob/master/setup.sh


# Variables

# DPDK version to use. If unspecified use master branch
export DPDK_VERSION=v17.05

# Path to the DPDK dir
export RTE_SDK=${HOME}/dpdk

# Target of build process
export RTE_TARGET=x86_64-native-linuxapp-gcc

# Name of network interface provisioned for DPDK to bind
# MARK: Which interface should I use to bind the DPDK?
export NET_IF_NAME=enp0s8

# Hugepages mount point
export HUGEPAGE_MOUNT=/mnt/huge

# Configure hugepages
# You can later check if this change was successful with `cat /proc/meminfo`
# Hugepages setup should be done as early as possible after boot
echo 64 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
sudo mkdir ${HUGEPAGE_MOUNT}
sudo mount -t hugetlbfs nodev ${HUGEPAGE_MOUNT}

# Set Hugepages in /etc/fstab so they persist across reboots
echo "hugetlbfs ${HUGEPAGE_MOUNT} hugetlbfs rw,mode=0777 0 0" | sudo tee -a /etc/fstab

# Install dependencies
sudo apt-get -qq update
sudo apt-get -y -qq install clang doxygen hugepages build-essential libnuma-dev libpcap-dev linux-headers-`uname -r`

# Get code from Git repo
git clone http://dpdk.org/git/dpdk ${RTE_SDK}

# Move to the DPDK dir
cd ${RTE_SDK}

# Checkout desired version, if specified
git checkout "${DPDK_VERSION:-master}"

# Config and build
make config T=${RTE_TARGET}
sed -ri 's,(PMD_PCAP=).*,\1y,' build/.config
make

# Install kernel modules
sudo modprobe uio
sudo insmod ${RTE_SDK}/build/kmod/igb_uio.ko

# Make uio and igb_uio installations persist across reboots
sudo ln -sf ${RTE_SDK}/build/kmod/igb_uio.ko /lib/modules/`uname -r`
sudo depmod -a
echo "uio" | sudo tee -a /etc/modules
echo "igb_uio" | sudo tee -a /etc/modules

## Bind secondary network adapter
## I need to set a second adapter in Vagrantfile
## Note that this NIC setup does not persist across reboots
#sudo ifconfig ${NET_IF_NAME} down
#sudo ${RTE_SDK}/usertools/dpdk-devbind.py --bind=igb_uio ${NET_IF_NAME}
#
## Add env variables setting to .profile file so that they are set at each login
#echo "export RTE_SDK=${RTE_SDK}" >> ${HOME}/.profile
#echo "export RTE_TARGET=${RTE_TARGET}" >> ${HOME}/.profile
#
## We need to do this to make the examples compile, not sure why.
#ln -sf ${RTE_SDK}/build ${RTE_SDK}/${RTE_TARGET}
