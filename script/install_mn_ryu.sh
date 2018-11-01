#! /bin/bash
#
# About: Install Mininet and Ryu SDN controller
#        Required for lightweight environment for NFV/SDN development
#
# MARK : Mininet does not fully support Python3 now, use Python2.7 instead
#

echo "Install Mininet..."
git clone git://github.com/mininet/mininet /home/vagrant/mininet
cd /home/vagrant/mininet/ || exit
git checkout -b 2.2.2 2.2.2
export PYTHON=python3
if [[ "$1" == "-min" ]]; then
    sudo bash ./util/install.sh -nf
else
    sudo bash ./util/install.sh -nfv
fi

echo "Install Ryu SDN controller..."
sudo apt update
sudo apt install -y gcc python3-dev libffi-dev libssl-dev libxml2-dev libxslt1-dev zlib1g-dev python3-pip
git clone git://github.com/osrg/ryu.git /home/vagrant/ryu
cd /home/vagrant/ryu || exit
git checkout -b v4.29 v4.29
sudo -H pip3 install .
