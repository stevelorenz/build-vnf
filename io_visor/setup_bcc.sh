#!/bin/bash
#
# About: Setup development environment for IO Visor project, bcc

BCC_VER="master"

if [[ $1 = '-b' ]]; then
    echo "[BCC] Install binaries."
    sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys D4284CDD
    echo "deb https://repo.iovisor.org/apt/xenial xenial main" | sudo tee /etc/apt/sources.list.d/iovisor.list
    sudo apt-get update
    sudo apt-get install bcc-tools libbcc-examples linux-headers-$(uname -r) python-pyroute2

elif [[ $1 = '-s' ]]; then
    # MARK: Used to follow the latest features. e.g. XDP redirecting
    echo "[BCC] Install from source code."
    echo "# Install requirements."
    # PPA for cmake3
    #sudo apt-get install -y software-properties-common
    #sudo add-apt-repository ppa:george-edison55/cmake-3.x
    sudo apt-get update
    sudo apt-get -y install bison build-essential cmake flex git libedit-dev \
        libllvm3.7 llvm-3.7-dev libclang-3.7-dev python zlib1g-dev libelf-dev
    sudo apt-get install -y linux-headers-$(uname -r) python-pyroute2
    # For Lua support
    sudo apt-get -y install luajit luajit-5.1-dev
    echo "# Compile BCC."
    git clone https://github.com/iovisor/bcc.git /home/vagrant/bcc
    cd /home/vagrant/bcc || exit
    git checkout -b "$BCC_VER"
    mkdir -p ./build
    cd ./build || exit
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make
    sudo make install
fi
