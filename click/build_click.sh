#! /bin/bash
#
# About: Build Click with customized elements in /elements/local

cd "$HOME/click" || exit
./configure --prefix=/usr/local --enable-local
sudo make clean
sudo make elemlist
sudo make install
