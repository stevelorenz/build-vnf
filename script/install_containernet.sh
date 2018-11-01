#! /bin/bash
#
#
# About: Install Containernet on Ubuntu
#        Use ansible with the default install.yml
# Mark : Use the fork of stevelorenz, plan to add virtio-user interfaces
#

sudo apt-get install -y ansible git aptitude
cd "$HOME" || exit
git clone https://github.com/stevelorenz/containernet.git
cd containernet || exit
cd ./ansible || exit
sudo ansible-playbook -i "localhost," -c local install.yml
cd .. || exit
sudo python setup.py install
