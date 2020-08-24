#!/bin/bash
#
# About: Setup development tools that need to be installed on the OS running containers.
#

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit
fi

echo "* Setup development tools for FFPP."
apt-get install -y python3-pip

echo "- Install Docker SDK for Python."
pip3 install docker==4.2.0

echo "- Install doxygen for API documentatin."
apt-get install -y doxygen

echo "- Install static code checker."
apt-get install -y cppcheck clang-format shellcheck
pip3 install flawfinder flake8 black
