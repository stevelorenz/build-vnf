FROM ubuntu:20.04

ENV TREX_VERSION="v2.81"
ENV XDP_TOOLS_VER="0.0.3"

LABEL RUN docker run --privileged --cap-add=ALL -v /mnt/huge:/mnt/huge -v /lib/modules:/lib/modules:ro -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev --name NAME -e NAME=NAME -e IMAGE=IMAGE IMAGE

RUN apt-get update --fix-missing \
    && DEBIAN_FRONTEND="noninteractive" apt-get install -y wget ca-certificates iproute2 net-tools telnet tcpdump iputils-ping procps \
       sudo build-essential pkg-config python3 python3-setuptools zlib1g-dev pciutils kmod strace \
       libnuma-dev libpcap-dev libelf-dev clang llvm gcc-multilib linux-headers-$(uname -r) linux-tools-common linux-tools-$(uname -r) \
       python3-numpy python3-scipy \
    && rm -rf /var/lib/apt/lists /var/cache/apt/archives

WORKDIR /opt
# Download and build xdp-tools. It has libbpf included which is required to build AF_XDP PMD driver for DPDK.
ENV XDP_TOOLS_DIR="/opt/xdp-tools"
RUN mkdir -p ${XDP_TOOLS_DIR}
RUN wget https://github.com/xdp-project/xdp-tools/releases/download/v${XDP_TOOLS_VER}/xdp-tools-${XDP_TOOLS_VER}.tar.gz && \
    tar -zxvf xdp-tools-${XDP_TOOLS_VER}.tar.gz -C ./xdp-tools --strip-components 1 && \
    cd ./xdp-tools && ./configure && make && make install && cd ./lib/libbpf/src && make install

# Let the build system and linker to find the libbpf.so
ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib64/pkgconfig
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/lib64

RUN mkdir /trex
WORKDIR /trex
# BUG: ERROR: cannot verify trex-tgn.cisco.com's certificate, issued by ‘CN=HydrantID SSL ICA G2,O=HydrantID (Avalanche Cloud Corporation),C=US’:
# As a temp-fix, --no-check-certificate is added...
RUN wget --no-check-certificate --no-cache https://trex-tgn.cisco.com/trex/release/${TREX_VERSION}.tar.gz && \
    tar -xzvf ./${TREX_VERSION}.tar.gz && \
    rm ./${TREX_VERSION}.tar.gz

COPY ./trex_cfg.yaml /etc/trex_cfg.yaml

WORKDIR /trex/${TREX_VERSION}

RUN ln -s /usr/bin/python3 /usr/bin/python && \
    tar -xzf trex_client_${TREX_VERSION}.tar.gz

# Remove unused files.
RUN rm -rf /opt/xdp-tools-${XDP_TOOLS_VER}.tar.gz

RUN mkdir -p /trex/${TREX_VERSION}/local
COPY ./run_basic_test.sh /trex/${TREX_VERSION}/local/run_basic_test.sh

CMD ["bash"]
