#!/bin/bash
#
# setup_devtools.sh
# About: Setup development tools that need to be installed on the OS running containers.
#

echo "* Setup development tools for FFPP."

echo "- Install doxygen for API documentatin."
apt-get install -y doxygen

echo "- Install static code checker..."
apt-get install -y python3-pip cppcheck clang-format
pip3 install flawfinder
