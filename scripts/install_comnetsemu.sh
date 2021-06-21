#!/bin/bash
#
# About: Install the minimum ComNetsEmu for network emulations.
#

if [ "$EUID" -eq 0 ]; then
    echo "Please run this script without sudo."
    exit
fi

set -o errexit
set -o nounset
set -o pipefail

if [[ "$#" -eq 0 ]]; then
    echo "- Install ComNetsEmu. Warning: already-installed ComNetsEmu is automatically removed."
    sudo apt-get update
    sudo apt-get install -y python3-pip
    if [ -d ~/comnetsemu ]; then sudo rm -rf ~/comnetsemu; fi
    if [ -d ~/comnetsemu_dependencies ]; then sudo rm -rf ~/comnetsemu_dependencies; fi

    git clone https://github.com/stevelorenz/comnetsemu.git ~/comnetsemu --branch dev
    cd ~/comnetsemu/util || exit
    bash ./install.sh -d
    bash ./install.sh -ncy
    cd ~/comnetsemu || exit
    sudo make develop

    echo "-- Build test containers."
    cd ~/comnetsemu/test_containers || exit
    sudo ./build.sh
    sudo docker image prune

    cp ~/comnetsemu/util/Xresources ~/.Xresources
elif [[ $1 == "-u" ]]; then
    echo "- Upgrade installed ComNetsEmu."
    cd ~/comnetsemu/util || exit
    bash ./install.sh -u
else
    echo "ERROR: Unknown option."
fi
