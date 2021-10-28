#!/bin/bash

if [[ -e ./config.sh ]]; then
    source ./config.sh
    echo "Number of hugepages-2048kB: $HUGEPAGE_NUM_2048"
else
    HUGEPAGE_NUM_2048=256
fi

mkdir -p /mnt/huge
(mount | grep hugetlbfs) >/dev/null || mount -t hugetlbfs nodev /mnt/huge
for i in {0..7}; do
    if [[ -e "/sys/devices/system/node/node$i" ]]; then
        echo $HUGEPAGE_NUM_2048 >/sys/devices/system/node/node$i/hugepages/hugepages-2048kB/nr_hugepages
    fi
done

echo "* Hugepage information: "
grep 'Huge' /proc/meminfo
