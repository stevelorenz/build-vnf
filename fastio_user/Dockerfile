#
# Dockerfile ONLY used for the development and tests of FastIO User library
# - root user is used
#
# - Dependencies:
#   - DPDK: v18.11 LTS
#

FROM ubuntu:18.04

# Build DPDK
# MARK: Use the latest LTS version

ENV RTE_SDK=/root/dpdk
ENV RTE_TARGET=x86_64-native-linuxapp-gcc
# Disable RDRAND, conflict with valgrind memory checker
ENV EXTRA_CFLAGS=-mno-rdrnd

RUN apt-get update && apt-get install -y net-tools \
        build-essential automake python-pip libcap-ng-dev gawk pciutils kmod \
        linux-headers-$(uname -a | awk '{print $3}') \
        git hugepages clang doxygen \
        libnuma-dev libpcap-dev

WORKDIR /root

RUN \
        git clone http://dpdk.org/git/dpdk "${RTE_SDK}" && \
        cd dpdk && \
        git checkout -b lts v18.11 && \
        make config T=${RTE_TARGET} && \
        make && \
        ln -sf "${RTE_SDK}/build" "${RTE_SDK}/${RTE_TARGET}"


# Install debug/perf tools
RUN apt-get install -y gdb linux-tools-$(uname -r) lldb valgrind iputils-ping iproute2 tcpdump

# Build FastIO User library
RUN git clone https://github.com/stevelorenz/build-vnf.git ./build-vnf && \
        cp -r /root/build-vnf/fastio_user/. /fastio_user
WORKDIR /fastio_user
RUN make clean && make lib

CMD ["bash"]
