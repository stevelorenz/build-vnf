#! /bin/bash
#
# setup.sh
#
# About: Setup SFC dev environment with Mininet
#

# Use latest stable versions
MN_VERSION="2.2.2"
RYU_VERSION="v4.29"

echo "# Start setting up SFC dev environment..."
echo "[WARN] Run this setup script on a newly created clean VM!"

echo "Install Ryu SDN controller ..."
sudo apt install -y gcc python-dev libffi-dev libssl-dev libxml2-dev libxslt1-dev zlib1g-dev python-pip
git clone git://github.com/osrg/ryu.git /home/vagrant/ryu
cd /home/vagrant/ryu || exit
git checkout -b $RYU_VERSION $RYU_VERSION
pip install .

echo "Install Mininet $MN_VERSION ..."
cd ~ || exit
git clone git://github.com/mininet/mininet /home/vagrant/mininet
cd /home/vagrant/mininet/ || exit
git checkout -b $MN_VERSION $MN_VERSION
bash ./util/install.sh -nfv

echo "# Setup finished."
