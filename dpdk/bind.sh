#!/bin/bash
#
# About: Bind the virtual interface to DPDK
#        SHOULD be executed after each reboot

export DPDK_IFCE=eth1

# Allocate hugepages
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Bind interface to DPDK
sudo ip link set ${DPDK_IFCE} down
echo
echo "# Interface status before binding:"
echo
python ${RTE_SDK}/usertools/dpdk-devbind.py --status

echo "# Bind ${DPDK_IFCE} to DPDK."
sudo python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=igb_uio ${DPDK_IFCE}

echo
echo "# Interface status after binding:"
python ${RTE_SDK}/usertools/dpdk-devbind.py --status
echo
