#! /bin/bash
#
# setup.sh
#
# About: Setup OpenDayLight SFC dev environment
#

export ODL_VERSION="0.9.0"

sudo apt-get update

# Install the default java runtime
sudo apt-get install default-jre-headless
echo "JAVA_HOME=\"/usr/lib/jvm/default-java\"" | sudo tee -a /etc/environment

# Install OpenDayLight
cd ~ || exit
wget https://nexus.opendaylight.org/content/repositories/public/org/opendaylight/integration/opendaylight/0.9.0/opendaylight-0.9.0.tar.gz
tar -xvf opendaylight-0.9.0.tar.gz

# Running the karaf distribution
# > Karaf is a container technology that allows developers to put all required software in a single distribution folder.
cd opendaylight-"$ODL_VERSION" || exit
echo "Run ODL with karaf..."
./bin/karaf
