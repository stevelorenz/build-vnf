#!/bin/bash
#
# setup_devtools.sh
# About: Setup development tools that need to be installed on the OS running containers.
#

echo "* Setup development tools for FFPP."

echo "- Install static code checker..."
apt-get install -y cppcheck clang-format
pip3 install flawfinder
