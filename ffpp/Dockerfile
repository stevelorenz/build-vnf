#
# Dockerfile ONLY used for the development and tests of FFPP library.
#
# This image includes also test and debug utilities and has a relative big size. (about 1.0GB)
# Most DPDK PMDs and features are disabled to reduce image size.
# It is not designed/suitable for deployment usage.
#

FROM ubuntu:18.04

#
# Build DPDK from source
#
ENV DPDK_VER=19.08
ENV RTE_SDK=/opt/dpdk-${DPDK_VER}
ENV RTE_TARGET=x86_64-native-linuxapp-gcc
# Disable RDRAND, conflict with valgrind memory checker
ENV EXTRA_CFLAGS="-g3 -Wno-error=maybe-uninitialized -fPIC -mno-rdrnd"

# Install build essentials for DPDK
RUN apt-get update && apt-get install -y wget build-essential \
    pciutils hugepages \
    libnuma-dev libpcap-dev xz-utils


RUN mkdir -p ${RTE_SDK}
WORKDIR /opt/
RUN wget http://fast.dpdk.org/rel/dpdk-${DPDK_VER}.tar.xz && \
    tar -xpvf dpdk-${DPDK_VER}.tar.xz

WORKDIR ${RTE_SDK}

RUN make config T=${RTE_TARGET}
# Use customized compile-time DPDK configuration
# The goal here is to reduce the image size
COPY config ${RTE_SDK}/build/.config

RUN make -j $(nproc) && \
    ln -sf "${RTE_SDK}/build" "${RTE_SDK}/${RTE_TARGET}"

#
# Build FFPP library
#
ENV FFPP_DIR /ffpp
# Install test utilities
RUN apt-get update && apt-get install -y gdb valgrind iputils-ping iproute2 python3 tcpdump net-tools && \
    rm -rf /var/lib/apt/lists/* /var/cache/apt/archives

RUN mkdir -p ${FFPP_DIR}
COPY . ${FFPP_DIR}
WORKDIR ${FFPP_DIR}
RUN make clean && make lib -j $(nproc)

CMD ["bash"]
