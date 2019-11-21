#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit
fi

echo "* Try to allocate 2MB hugepages..."
bash ../util/setup_hugepage.sh

echo "* Load MSR CPUID kernel modules..."
modprobe msr
modprobe cpuid
