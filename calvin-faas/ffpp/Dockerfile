#
# This image is ONLY used for the development and tests of FFPP library.
#
# TODO (Zuo): The image size is too big currently... Reduce it later.
#      - Adjust DPDK_BUILD_OPTION to remove unused PMD drivers or library.
#      - Use multi-stage build for deployment.
#

FROM ubuntu:20.04 as builder

ENV XDP_TOOLS_VER="0.0.3"

ENV DPDK_VER="20.02"
ENV RTE_SDK="/opt/dpdk"
ENV RTE_TARGET="x86_64-native-linuxapp-gcc"
# Do not build tests for DPDK libraries, reduce image size.
ENV DPDK_BUILD_OPTION="-Dtests=false"

# Install build essentials for DPDK and XDP.
# - DPDK 20.02 uses Meson and Ninja as its default build system (instead of its plan makefile based system).
# - Clang LLVM are required to build XDP program.
RUN apt-get update && \
    DEBIAN_FRONTEND="noninteractive" apt-get install -y wget build-essential pkg-config python3 meson ninja-build \
    libnuma-dev libpcap-dev gcovr \
    libelf-dev clang llvm gcc-multilib linux-headers-5.4.0-28-generic linux-tools-common linux-tools-5.4.0-28-generic \
    libcmocka-dev iproute2 iputils-ping tcpdump \
    bash-completion cppcheck clang-tidy

WORKDIR /opt/

# Download and build xdp-tools. It has libbpf included which is required to build AF_XDP PMD driver for DPDK.
ENV XDP_TOOLS_DIR="/opt/xdp-tools"
RUN mkdir -p ${XDP_TOOLS_DIR}
RUN wget https://github.com/xdp-project/xdp-tools/releases/download/v${XDP_TOOLS_VER}/xdp-tools-${XDP_TOOLS_VER}.tar.gz && \
    tar -zxvf xdp-tools-${XDP_TOOLS_VER}.tar.gz -C ./xdp-tools --strip-components 1 && \
    cd ./xdp-tools && ./configure && make && make install && cd ./lib/libbpf/src && make install

RUN mkdir -p ${RTE_SDK}
RUN wget http://fast.dpdk.org/rel/dpdk-${DPDK_VER}.tar.xz && \
    tar -xJf dpdk-${DPDK_VER}.tar.xz -C ./dpdk --strip-components 1 && \
    cd ./dpdk && meson ${DPDK_BUILD_OPTION} build && cd build && \
    ninja && ninja install && ldconfig

# Let the build system and linker to find the libbpf.so
ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib64/pkgconfig
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/lib64

# Remove unused files.
RUN rm -rf /opt/xdp-tools-${XDP_TOOLS_VER}.tar.gz /opt/dpdk-${DPDK_VER}.tar.xz

# For Rust env.
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y rustc

# Build FFPP library
ENV FFPP_PATH /ffpp
RUN mkdir -p ${FFPP_PATH}
COPY . ${FFPP_PATH}
WORKDIR ${FFPP_PATH}
RUN make release && make install

# ONLY used for performance benchmarking
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y sockperf

# APT cleanup.
RUN rm -rf /var/lib/apt/lists /var/cache/apt/archives

CMD ["bash"]
