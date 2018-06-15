#!/bin/bash
# About: Update Linux Kernel on Ubuntu 16.04 for XDP support
#        For BPF features, check:
#           https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md
# MARK :
#        - XDP 4.8
#        - AF_XDP 4.18
#        - XDP REDIRECT works on 4.17

cd /tmp
# V4.14
#wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.14/linux-headers-4.14.0-041400_4.14.0-041400.201711122031_all.deb
#wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.14/linux-headers-4.14.0-041400-generic_4.14.0-041400.201711122031_amd64.deb
#wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.14/linux-image-4.14.0-041400-generic_4.14.0-041400.201711122031_amd64.deb

# V4.17
wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.17/linux-headers-4.17.0-041700_4.17.0-041700.201806041953_all.deb
wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.17/linux-modules-4.17.0-041700-generic_4.17.0-041700.201806041953_amd64.deb
wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.17/linux-headers-4.17.0-041700-generic_4.17.0-041700.201806041953_amd64.deb
wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.17/linux-image-unsigned-4.17.0-041700-generic_4.17.0-041700.201806041953_amd64.deb

# Install libssl1.1 from https://packages.ubuntu.com/bionic/amd64/libssl1.1/download
sudo echo "deb http://cz.archive.ubuntu.com/ubuntu bionic main" >> /etc/apt/sources.list
sudo apt update
sudo apt install -y libssl1.1

sudo dpkg -i *.deb
# Update initramfs and grub for new kernel
sudo update-initramfs -u -k 4.17.0-041700-generic
sudo update-grub
grep -A100 submenu  /boot/grub/grub.cfg |grep menuentry
