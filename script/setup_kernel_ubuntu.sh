#!/bin/bash
# About: Install and setup Linux kernel on Ubuntu
#

if [[ $1 = '-h' || $1 = '--help' ]]; then
    echo "Usage: ./setup_kernel_ubuntu.sh -kernel_version"
    echo "kernel_version: 4.18, 4.19"
    exit 0
elif [[ $1 != '4.18' && $1 != '4.19' ]]; then
    echo "Invalid kernel version."
    exit 1
fi

# Install libssl1.1 from https://packages.ubuntu.com/bionic/amd64/libssl1.1/download
echo "deb http://cz.archive.ubuntu.com/ubuntu bionic main" | sudo tee -a /etc/apt/sources.list > /dev/null
sudo apt update
sudo apt install -y libssl1.1

cd /tmp || exit

if [[ $1 = '4.18' ]]; then
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-headers-4.18.0-041800rc4_4.18.0-041800rc4.201807082030_all.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-modules-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-headers-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-image-unsigned-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb
    sudo dpkg -i *.deb
    sudo update-initramfs -u -k 4.18.0-041800rc4-generic
fi

if [[ $1 == '4.19' ]]; then
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.19/linux-headers-4.19.0-041900_4.19.0-041900.201810221809_all.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.19/linux-headers-4.19.0-041900-generic_4.19.0-041900.201810221809_amd64.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.19/linux-image-unsigned-4.19.0-041900-generic_4.19.0-041900.201810221809_amd64.deb
    wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.19/linux-modules-4.19.0-041900-generic_4.19.0-041900.201810221809_amd64.deb
    sudo dpkg -i *.deb
    sudo update-initramfs -u -k 4.19.0-041900-generic
fi

sudo update-grub
grep -A100 submenu  /boot/grub/grub.cfg |grep menuentry
