#
# This image is ONLY used for the DEVELOPMENT and TESTS of FFPP library.
#
# TODO (Zuo): Reduce the image size if possible.
#

FROM ubuntu:20.04

# Install dependencies for DPDK and XDP
# - Clang LLVM are required to build XDP programs.
RUN apt-get update && \
    DEBIAN_FRONTEND="noninteractive" apt-get install -y wget build-essential pkg-config python3 meson ninja-build \
    libnuma-dev libpcap-dev \
    libelf-dev clang llvm gcc-multilib linux-tools-common

WORKDIR /opt/

# Build xdp-tools from source (no deb package available)
# xdp-tools has libbpf included which is required to build AF_XDP PMD driver of DPDK.
ENV XDP_TOOLS_VER="0.0.3"
ENV XDP_TOOLS_DIR="/opt/xdp-tools"
RUN mkdir -p ${XDP_TOOLS_DIR} && \
    wget https://github.com/xdp-project/xdp-tools/releases/download/v${XDP_TOOLS_VER}/xdp-tools-${XDP_TOOLS_VER}.tar.gz && \
    tar -zxvf xdp-tools-${XDP_TOOLS_VER}.tar.gz -C ./xdp-tools --strip-components 1 && \
    cd ./xdp-tools && ./configure && make && make install && cd ./lib/libbpf/src && make install
# Let the build system and linker to find the libbpf.so
ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib64/pkgconfig
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/lib64

# Build DPDK from source.
# The deb package (of Ubuntu 20.04) is not used currently since it does not
# contain AF_XDP PMD. Will use the deb package when AF_XDP is fully supported.
ENV DPDK_VER="20.11"
ENV DPDK_BUILD_OPTION="-Dtests=false \
-Ddisable_drivers=baseband/*,bus/ifpga,bus/fslmc,bus/dpaa,\
event/dsw,event/opdl,event/octeontx,event/octeontx2,\
net/ark,net/avp,net/atlantic,net/bond,net/bnxt,net/bnx2x,net/cxgbe,net/ena,net/enetc,\
net/failsafe,net/fm10k,net/hinic,net/hns3,net/ice,net/kni,net/liquidio,net/mlx5,net/octeontx,net/octeontx2,\
net/qede,net/sfc,net/softnic,net/thunderx,net/vmxnet3,\
crypto/nitrox,crypto/octeontx_crypto,crypto/octeontx2_crypto,\
compress/octeontx_compress,\
vdpa/*\
"
ENV RTE_SDK="/opt/dpdk"
ENV RTE_TARGET="x86_64-native-linuxapp-gcc"
RUN mkdir -p ${RTE_SDK} && \
    wget http://fast.dpdk.org/rel/dpdk-${DPDK_VER}.tar.xz && \
    tar -xJf dpdk-${DPDK_VER}.tar.xz -C ./dpdk --strip-components 1 && \
    cd ./dpdk && meson ${DPDK_BUILD_OPTION} build && cd build && \
    ninja && ninja install && ldconfig

# Remove unused files.
RUN rm -rf /opt/xdp-tools-${XDP_TOOLS_VER}.tar.gz /opt/dpdk-${DPDK_VER}.tar.xz

# Install FFPP dependencies and dev tools.
RUN debian_frontend="noninteractive" apt-get install -y \
    libczmq-dev libjansson-dev golang \
    libcmocka-dev libcpufreq-dev gcovr libmsgsl-dev python3-pybind11 \
    python3-pip python3-zmq python3-dev python3-numpy \
    bash-completion cppcheck clang-tidy net-tools iproute2 iputils-ping tcpdump

RUN pip3 install -q docker cffi

# Build FFPP library
ENV FFPP_PATH /ffpp
RUN mkdir -p ${FFPP_PATH}
COPY . ${FFPP_PATH}
WORKDIR ${FFPP_PATH}
RUN make release && make install
ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/x86_64-linux-gnu/pkgconfig
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib/x86_64-linux-gnu

# APT cleanup.
RUN rm -rf /var/lib/apt/lists /var/cache/apt/archives

WORKDIR ${FFPP_PATH}

CMD ["bash"]
