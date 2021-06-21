#!/bin/bash
#
# About: Configure GRUB parameters
#
#        - Allocate hugepage during boot
#        - Isolate CPU cores from Linux kernel scheduler
#

function config_grub() {
    dt=$(date '+%d_%m_%Y')
    echo "# Backup original GRUB config file"
    cp /etc/default/grub "/etc/default/grub.backup.$dt"

    echo "# Update GRUB_CMDLINE_LINUX_DEFAULT in grub"
    # MARK: Change the options if you want to use 1G hugepages and isolate other cores
    # e.g. Hugepagesz=1G isolcpus=1,2,3,4 (the first core has index 0)
    sed -i 's/^GRUB_CMDLINE_LINUX_DEFAULT="/&hugepagesz=2M hugepages=256 isolcpus=1 /' /etc/default/grub

    echo "# Update GRUB"
    update-grub
}

if [[ $1 == "-c" ]]; then
    echo "# Check the current settings, please run this after configure GRUB and reboot."
    echo "* Isolated CPU cores:"
    cat /sys/devices/system/cpu/isolated
    echo "* Hugepages: "
    grep 'HugePages' /proc/meminfo
else
    config_grub
fi
