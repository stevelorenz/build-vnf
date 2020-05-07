#
# This image is ONLY used for the development and tests of FFPP library.
#
# TODO (Zuo): The image size is too big currently... Reduce it later.
#

FROM ubuntu:20.04 as builder

ENV LIBBPF_VER=v0.0.8

ENV DPDK_VER=20.02
ENV RTE_SDK=/opt/dpdk
ENV RTE_TARGET=x86_64-native-linuxapp-gcc
ENV DPDK_BUILD_OPTION=

# Install build essentials for DPDK and XDP.
# - DPDK 20.02 uses Meson and Ninja as its default build system (instead of its plan makefile based system).
# - Clang LLVM are required to build XDP program.
RUN apt-get update && \
    DEBIAN_FRONTEND="noninteractive" apt-get install -y wget build-essential pkg-config python3 meson ninja-build \
    libnuma-dev libpcap-dev \
    libelf-dev clang llvm gcc-multilib linux-tools-common linux-tools-generic linux-headers-$(uname -r) \
    bash-completion && \
    rm -rf /var/lib/apt/lists /var/cache/apt/archives

WORKDIR /opt/

# Download and build libbpf. It is required to build AF_XDP PMD driver for DPDK.
RUN mkdir -p /opt/libbpf
RUN wget https://github.com/libbpf/libbpf/archive/${LIBBPF_VER}.tar.gz && \
    tar -zxvf ${LIBBPF_VER}.tar.gz -C ./libbpf --strip-components 1 && \
    cd ./libbpf/src && make && make install

RUN mkdir -p ${RTE_SDK}
RUN wget http://fast.dpdk.org/rel/dpdk-${DPDK_VER}.tar.xz && \
    tar -xJf dpdk-${DPDK_VER}.tar.xz -C ./dpdk --strip-components 1 && \
    cd ./dpdk && meson ${DPDK_BUILD_OPTION} build && cd build && \
    ninja && ninja install && ldconfig

CMD ["bash"]
