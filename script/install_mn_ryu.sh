#! /bin/bash
#
# About: Install Mininet and Ryu SDN controller
#        For Python 3
#

echo "Install Mininet..."
git clone git://github.com/mininet/mininet /home/vagrant/mininet
cd /home/vagrant/mininet/
git checkout -b 2.2.2 2.2.2
export PYTHON=python3
sudo bash ./util/install.sh -nfv

echo "Install Ryu SDN controller..."
sudo apt install -y gcc python3-dev libffi-dev libssl-dev libxml2-dev libxslt1-dev zlib1g-dev python3-pip
git clone git://github.com/osrg/ryu.git /home/vagrant/ryu
cd /home/vagrant/ryu
git checkout -b v4.29 v4.29
sudo -H pip3 install .
