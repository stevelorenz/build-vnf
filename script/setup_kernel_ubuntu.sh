#!/bin/bash
# About: Install and setup Linux kernel on Ubuntu
#

if [[ $1 = '-h' || $1 = '--help' ]]; then
    echo "Usage: ./setup_kernel_ubuntu.sh -kernel_version"
    echo "kernel_version: 4.17, 4.18"
    exit 0
elif [[ $1 != '4.17' && $1 != '4.18' ]]; then
    echo "Invalid kernel version."
    exit 1
fi

# Install libssl1.1 from https://packages.ubuntu.com/bionic/amd64/libssl1.1/download
echo "deb http://cz.archive.ubuntu.com/ubuntu bionic main" | sudo tee -a /etc/apt/sources.list > /dev/null
sudo apt update
sudo apt install -y libssl1.1

cd /tmp || exit
if [[ $1 = '4.17' ]]; then
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.17/linux-headers-4.17.0-041700_4.17.0-041700.201806041953_all.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.17/linux-modules-4.17.0-041700-generic_4.17.0-041700.201806041953_amd64.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.17/linux-headers-4.17.0-041700-generic_4.17.0-041700.201806041953_amd64.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.17/linux-image-unsigned-4.17.0-041700-generic_4.17.0-041700.201806041953_amd64.deb
    sudo dpkg -i *.deb
    sudo update-initramfs -u -k 4.17.0-041700-generic
fi

if [[ $1 = '4.18' ]]; then
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-headers-4.18.0-041800rc4_4.18.0-041800rc4.201807082030_all.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-modules-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-headers-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-image-unsigned-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb
    sudo dpkg -i *.deb
    sudo update-initramfs -u -k 4.18.0-041800rc4-generic
fi

sudo update-grub
grep -A100 submenu  /boot/grub/grub.cfg |grep menuentry
