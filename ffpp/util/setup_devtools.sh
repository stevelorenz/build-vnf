#!/bin/bash
#
# setup_devtools.sh
#

echo "* Setup development tools for FFPP"

echo "- Install static code checker..."
apt-get install -y cppcheck clang-format
pip3 install flawfinder scapy
