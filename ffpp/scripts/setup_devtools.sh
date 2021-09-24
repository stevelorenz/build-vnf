#!/bin/bash
#
# About: Setup development tools that need to be installed on the OS running containers.
#

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit
fi

DEBIAN_FRONTEND=noninteractive apt-get install -y \
    black \
    clang-format \
    clang-tidy \
    cppcheck \
    doxygen \
    flawfinder \
    gcovr \
    python3-flake8 \
    shellcheck \
    valgrind
