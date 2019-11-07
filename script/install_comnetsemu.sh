#!/bin/bash
#
# About: Install the minimum ComNetsEmu for FFPP tests
#

if [ -d ~/comnetsemu ]; then rm -rf ~/comnetsemu; fi
if [ -d ~/comnetsemu_dependencies ]; then sudo rm -rf ~/comnetsemu_dependencies; fi

git clone https://bitbucket.org/comnets/comnetsemu/src/master/ ~/comnetsemu
cd ~/comnetsemu/util || exit
bash ./install.sh -nc
cd ~/comnetsemu || exit
sudo make develop
