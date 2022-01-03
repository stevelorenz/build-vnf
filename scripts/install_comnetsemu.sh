#!/bin/bash
#
# About: Install the minimum ComNetsEmu for network emulations.
#

if [ "$EUID" -eq 0 ]; then
    echo "Please run this script without sudo or not as root."
    exit
fi

set -o errexit
set -o nounset
set -o pipefail

if [[ "$#" -eq 0 ]]; then
    echo "- Install ComNetsEmu with sources located in ~/comnetsemu directory."
	echo "Warning: ~/comnetsemu and ~/comnetsemu_dependencies directories are auto removed if they already exist."
    if [ -d ~/comnetsemu ]; then sudo rm -rf ~/comnetsemu; fi
    if [ -d ~/comnetsemu_dependencies ]; then sudo rm -rf ~/comnetsemu_dependencies; fi

    git clone https://github.com/stevelorenz/comnetsemu.git ~/comnetsemu --branch dev
    cd ~/comnetsemu/util || exit
    bash ./install.sh -a

	echo "- Copy and merge the Xresources (for a better Xterm theme)"
    cp ~/comnetsemu/util/Xresources ~/.Xresources

elif [[ $1 == "-u" ]]; then
    echo "- Upgrade installed ComNetsEmu."
    cd ~/comnetsemu/util || exit
    bash ./install.sh -u
else
    echo "ERROR: Unknown option."
fi
