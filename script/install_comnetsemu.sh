#!/bin/bash
#
# About: Install the minimum ComNetsEmu for FFPP tests
#

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
