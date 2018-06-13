#!/bin/bash
# About: Update Linux Kernel on Ubuntu 16.04 for XDP support
#        For BPF features, check:
#           https://github.com/iovisor/bcc/blob/master/docs/kernel-versions.md
# MARK :
#        - XDP 4.8
#        - AF_XDP 4.18

cd /tmp
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.14/linux-headers-4.14.0-041400_4.14.0-041400.201711122031_all.deb
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.14/linux-headers-4.14.0-041400-generic_4.14.0-041400.201711122031_amd64.deb
wget http://kernel.ubuntu.com/~kernel-ppa/mainline/v4.14/linux-image-4.14.0-041400-generic_4.14.0-041400.201711122031_amd64.deb

sudo dpkg -i *.deb
sudo update-grub
grep -A100 submenu  /boot/grub/grub.cfg |grep menuentry
