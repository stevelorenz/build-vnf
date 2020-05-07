#!/bin/bash
#
# About: Setup BPF Compiler Collection (BCC)
#

BCC_BRANCH="master"

if [[ $1 == '-p' ]]; then
    echo "[BCC] Install nightly packages."
    echo "deb [trusted=yes] https://repo.iovisor.org/apt/bionic bionic-nightly main" | sudo tee /etc/apt/sources.list.d/iovisor.list
    sudo apt-get update
    sudo apt-get install -y bcc-tools linux-headers-"$(uname -r)" python-pyroute2

elif [[ $1 == '-s' ]]; then
    # MARK: Used to follow the latest features. e.g. XDP redirecting
    echo "[BCC] Compile and install from source code. Build Python3 bindings"
    echo "# Install requirements."
    # PPA for cmake3
    #sudo apt-get install -y software-properties-common
    #sudo add-apt-repository ppa:george-edison55/cmake-3.x
    sudo apt-get update
    sudo apt-get -y install bison build-essential cmake flex git libedit-dev \
        libllvm3.7 llvm-3.7-dev libclang-3.7-dev python zlib1g-dev libelf-dev
    sudo apt-get install -y linux-headers-"$(uname -r)" python-pyroute2 python3-dev python3-pyroute2
    # For Lua support
    sudo apt-get -y install luajit luajit-5.1-dev
    echo "# Compile BCC."
    git clone https://github.com/iovisor/bcc.git "$HOME/bcc"
    cd "$HOME/bcc" || exit
    git checkout -b "$BCC_BRANCH"
    mkdir -p ./build
    cd ./build || exit
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DPYTHON_CMD=python3
    make
    echo "# Install BCC"
    sudo make install
else
    echo "Use -p [from package] or -s [from source] option to install BCC."
fi
