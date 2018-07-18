#! /bin/bash
#
# About: Install gcc 8.0 on Ubuntu 14.04
#        This is needed if NCkernel is compiled with gcc 8.0, libasan.so.5 is required

sudo add-apt-repository -y ppa:jonathonf/gcc
sudo apt-get update
sudo apt-get install gcc-8
