#! /bin/bash
#
# About: Build libdpdkhelper.so using a docker environment
#

echo "Build dpdk image with ./Dockerfile.dpdk ..."
sudo docker build -t dpdk --file ../../dockerfiles/Dockerfile.dpdk .

sudo docker run --rm \
    -v $PWD:/dpdk_helper -w /dpdk_helper -t dpdk make

cp ./build/lib/libdpdkhelper.so ./
