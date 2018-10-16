#! /bin/bash
#
# setup.sh
#
# About: Setup SFC dev environment with Ryu SDN controller
#

# Use latest stable versions
MN_VERSION="2.2.2"

echo "Install Ryu SDN controller ..."
sudo apt install -y gcc python-dev libffi-dev libssl-dev libxml2-dev libxslt1-dev zlib1g-dev python-pip
git clone git://github.com/osrg/ryu.git /home/vagrant/ryu
cd /home/vagrant/ryu || exit
pip install .

echo "Install Mininet $MN_VERSION ..."
cd ~ || exit
git clone git://github.com/mininet/mininet /home/vagrant/mininet
cd /home/vagrant/mininet/ || exit
git checkout -b $MN_VERSION $MN_VERSION
bash ./util/install.sh -nfv

echo "Download sfc_app..."
cd ~ || exit
git clone https://github.com/abulanov/sfc_app.git
