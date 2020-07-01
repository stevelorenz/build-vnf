#!/bin/bash
#
# About: Install the minimum ComNetsEmu for FFPP tests
#

if [[ $1 == "-u" ]]; then
    echo "- Upgrade installed ComNetsEmu."
    cd ~/comnetsemu/util || exit
    bash ./install.sh -u
else
    echo "- Install ComNetsEmu. Warning: already-installed ComNetsEmu is automatically removed."
    sudo apt-get update
    sudo apt-get install -y python3-pip
    if [ -d ~/comnetsemu ]; then sudo rm -rf ~/comnetsemu; fi
    if [ -d ~/comnetsemu_dependencies ]; then sudo rm -rf ~/comnetsemu_dependencies; fi

    git clone https://git.comnets.net/public-repo/comnetsemu.git ~/comnetsemu
    cd ~/comnetsemu/util || exit
    bash ./install.sh -d
    bash ./install.sh -nc
    cd ~/comnetsemu || exit
    sudo make develop

    echo "-- Build test containers."
    cd ~/comnetsemu/test_containers || exit
    sudo ./build.sh
    sudo docker image prune
fi
