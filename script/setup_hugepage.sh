#! /bin/bash
#
# About: Setup hugepages after booting
#


if [[ -z $1 ]]; then
    echo "Missing number of hugepages to be allocated!"
    echo "Usage: sudo ./setup_hugepage.sh page_number"
    exit 1
fi

mkdir -p /mnt/huge
(mount | grep hugetlbfs) > /dev/null || mount -t hugetlbfs nodev /mnt/huge
for i in {0..7}
do
    if [[ -e "/sys/devices/system/node/node$i" ]]
    then
        echo $1 > /sys/devices/system/node/node$i/hugepages/hugepages-2048kB/nr_hugepages
    fi
done
