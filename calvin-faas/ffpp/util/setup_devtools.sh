#!/bin/bash
#
# setup_devtools.sh
# About: Setup development tools that need to be installed on the OS running containers.
#

echo "* Setup development tools for FFPP."
apt-get install -y python3-pip
pip3 install --user docker==4.2.0

echo "- Install Docker SDK for Python."

echo "- Install doxygen for API documentatin."
apt-get install -y doxygen

echo "- Install static code checker."
apt-get install -y cppcheck clang-format
pip3 install --user flawfinder
