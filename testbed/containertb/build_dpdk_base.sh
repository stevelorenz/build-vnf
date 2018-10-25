#! /bin/bash
#
# About: Build DPDK inside docker container
#

# Path to the DPDK dir
export RTE_SDK=${HOME}/dpdk

# Target of build process
export RTE_TARGET=x86_64-native-linuxapp-gcc

# Config and build DPDK
# get code from git repo
git clone http://dpdk.org/git/dpdk ${RTE_SDK}
cd "${RTE_SDK}" || exit

if [[ $1 = 'stable' ]]; then
    export DPDK_VERSION=v18.02-rc4
    git checkout -b dev ${DPDK_VERSION}
    echo "# Use stable branch: $DPDK_VERSION"
else
    echo "# Use the master branch."
fi
make config T=${RTE_TARGET}
sed -ri 's,(PMD_PCAP=).*,\1y,' build/.config
echo "# Compiling dpdk target from source."
make

cd $RTE_SDK
make -C examples RTE_SDK=$(pwd) RTE_TARGET=build o=$(pwd)/build/examples

echo "# Link built binaries."
ln -sf ${RTE_SDK}/build ${RTE_SDK}/${RTE_TARGET}
