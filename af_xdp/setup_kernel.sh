#!/bin/bash
# About: Install Linux kernel V4.18-rc4
#

cd /tmp

wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-headers-4.18.0-041800rc4_4.18.0-041800rc4.201807082030_all.deb
wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-modules-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb
wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-headers-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb
wget -c http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.18-rc4/linux-image-unsigned-4.18.0-041800rc4-generic_4.18.0-041800rc4.201807082030_amd64.deb

# Install libssl1.1 from https://packages.ubuntu.com/bionic/amd64/libssl1.1/download
echo "deb http://cz.archive.ubuntu.com/ubuntu bionic main" | sudo tee -a /etc/apt/sources.list > /dev/null
sudo apt update
sudo apt install -y libssl1.1

sudo dpkg -i *.deb
# Update initramfs and grub for new kernel
sudo update-initramfs -u -k 4.18.0-041800rc4-generic
sudo update-grub
grep -A100 submenu  /boot/grub/grub.cfg |grep menuentry
