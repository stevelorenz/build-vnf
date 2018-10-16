#!/bin/bash
# About: Install Mininet, used for emulations

git clone git://github.com/mininet/mininet /home/vagrant/mininet
cd /home/vagrant/mininet || exit
git checkout -b 2.2.1
bash ./util/install.sh -a

sudo mn --test pingall
