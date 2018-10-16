#!/bin/bash
#
# About: Bind virtual interfaces to DPDK
#        SHOULD be executed after each reboot

export INGRESS_IFCE=eth1
export EGRESS_IFCE=eth2

DPDK_IFCE_INFO_DIR=${HOME}/.local/share/dpdk/
mkdir -p ${DPDK_IFCE_INFO_DIR}
DPDK_IFCE_INFO_FILE=${DPDK_IFCE_INFO_DIR}/ifce_info.txt

# Allocate hugepages
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Record interface infos before binding
ip addr show ${INGRESS_IFCE} > ${DPDK_IFCE_INFO_FILE}
ip addr show ${EGRESS_IFCE} >> ${DPDK_IFCE_INFO_FILE}

# Bind interface to DPDK
sudo ip link set ${INGRESS_IFCE} down
sudo ip link set ${EGRESS_IFCE} down

echo
echo "# Interface status before binding:"
echo
python ${RTE_SDK}/usertools/dpdk-devbind.py --status

echo "# Bind ${INGRESS_IFCE} to DPDK."
sudo python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=igb_uio ${INGRESS_IFCE}
sudo python ${RTE_SDK}/usertools/dpdk-devbind.py --bind=igb_uio ${EGRESS_IFCE}

echo
echo "# Interface status after binding:"
python ${RTE_SDK}/usertools/dpdk-devbind.py --status
echo
