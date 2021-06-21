#! /usr/bin/env bash
#
# About: Setup hugepages after booting.
#        Alternative (better) approach is to use kernel parameter.
#

HUGEPAGE_NUM_2048=256

echo 3 | tee /proc/sys/vm/drop_caches

mkdir -p /mnt/huge
(mount | grep hugetlbfs) >/dev/null || mount -t hugetlbfs nodev /mnt/huge
for i in {0..7}; do
    if [[ -e "/sys/devices/system/node/node$i" ]]; then
        echo $HUGEPAGE_NUM_2048 >/sys/devices/system/node/node"$i"/hugepages/hugepages-2048kB/nr_hugepages
    fi
done

grep 'Huge' /proc/meminfo
