#!/bin/bash
#
# install_valgrind.sh
# 
# Valgrind with version >= 3.18.1 is needed to run DPDK applications.
# The version in the Ubuntu repo is not new enough currently.
# This script install Valgrind with a deb package as temporary workaround...
# Will be removed when the Valgrind version in the Docker image is new enough
#
#

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit
fi

apt-get remove -y valgrind

cd /tmp/ || exit
wget http://ftp.de.debian.org/debian/pool/main/v/valgrind/valgrind_3.18.1-1_amd64.deb
dpkg -i ./valgrind_3.18.1-1_amd64.deb
rm ./valgrind_3.18.1-1_amd64.deb
