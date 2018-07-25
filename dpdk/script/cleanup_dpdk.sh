#!/bin/bash
#
# About: Cleanup already installed DPDK environment.
#        Used to update DPDK version
# Email: xianglinks@gmail.com

###############
#  Variables  #
###############

# DPDK version to use. If unspecified use master branch
export DPDK_VERSION=v18.02-rc4

# Path to the DPDK dir
export RTE_SDK=${HOME}/dpdk

# Target of build process
export RTE_TARGET=x86_64-native-linuxapp-gcc

# Enable debugging with gdb
# MARK: This add debug info also in testpmd app
# export EXTRA_CFLAGS += -O2 -pg -g

# Hugepages mount point
export HUGEPAGE_MOUNT=/mnt/huge

#####################
#  Cleanup Process  #
#####################

echo 0 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
sudo umount ${HUGEPAGE_MOUNT}
sudo rm -rf ${HUGEPAGE_MOUNT}

sed -i '$ d' /etc/fstab
rm -rf ${RTE_SDK}

# Install and load kernel modules
echo "# Loading kernel modules."
sudo modprobe -r uio
sudo rmmod igb_uio
sudo rm /lib/modules/`uname -r`/igb_uio.ko
sudo depmod -a
echo "uio" | sudo tee -a /etc/modules
echo "igb_uio" | sudo tee -a /etc/modules
echo "" | sudo tee /etc/modules

echo ""
echo "# Cleanup finished."
