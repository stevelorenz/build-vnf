#! /bin/bash
#
# About: Load specific Kernel modules
#
#

if [[ $1 == "kni" ]]; then
    echo "# Load DPDK KNI Kmod"
    sudo insmod ${RTE_SDK}/build/kmod/rte_kni.ko
    sudo lsmod | grep rte_kni
fi
