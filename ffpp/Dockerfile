#
# This image is ONLY used for the DEVELOPMENT and TESTS of FFPP library.
#
# TODO (Zuo): Reduce the image size. Currently 1.6 GB.
#      - Use multi-stage build for deployment. Use a seperate Dockerfile.deploy.
#

FROM ubuntu:20.04

# Install build essentials for DPDK and XDP.
# - Clang LLVM are required to build XDP program.
RUN apt-get update && \
    DEBIAN_FRONTEND="noninteractive" apt-get install -y wget build-essential pkg-config python3 meson ninja-build \
    libnuma-dev libpcap-dev gcovr \
    libelf-dev clang llvm gcc-multilib linux-headers-$(uname -r) linux-tools-common linux-tools-$(uname -r) \
    libcmocka-dev iproute2 iputils-ping tcpdump \
    bash-completion cppcheck clang-tidy libcpufreq-dev

WORKDIR /opt/

ENV XDP_TOOLS_VER="0.0.3"
# Download and build xdp-tools. It has libbpf included which is required to build AF_XDP PMD driver for DPDK.
ENV XDP_TOOLS_DIR="/opt/xdp-tools"
RUN mkdir -p ${XDP_TOOLS_DIR}
RUN wget https://github.com/xdp-project/xdp-tools/releases/download/v${XDP_TOOLS_VER}/xdp-tools-${XDP_TOOLS_VER}.tar.gz && \
    tar -zxvf xdp-tools-${XDP_TOOLS_VER}.tar.gz -C ./xdp-tools --strip-components 1 && \
    cd ./xdp-tools && ./configure && make && make install && cd ./lib/libbpf/src && make install

ENV DPDK_VER="20.08"
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

# Install FFPP dependencies.
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y \
    python3-pip libczmq-dev python3-zmq libjansson-dev python3-dev python3-numpy \
    git
RUN pip3 install -q docker cffi conan

# Build FFPP library
ENV FFPP_PATH /ffpp
RUN mkdir -p ${FFPP_PATH}
COPY . ${FFPP_PATH}
WORKDIR ${FFPP_PATH}
RUN make release && make install
WORKDIR ${FFPP_PATH}/ffpp-py
RUN python3 setup.py install

# APT cleanup.
RUN rm -rf /var/lib/apt/lists /var/cache/apt/archives

CMD ["bash"]
