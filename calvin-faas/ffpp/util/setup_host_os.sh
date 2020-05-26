#!/bin/bash

if [[ -z ${XDP_TOOLS_DIR} ]]; then
    XDP_TOOLS_DIR="/opt/xdp-tools"
fi

if [[ -z ${XDP_TOOLS_VER} ]]; then
    XDP_TOOLS_VER="0.0.3"
fi

if [[ -d ${XDP_TOOLS_DIR} ]]; then
    echo "ERR: XDP directory already exists."
    exit 1
fi

echo "* Download, build and install XDP-Tools (version ${XDP_TOOLS_VER})."
echo "- Dependency packages are also installed."
echo "- Source is downloaded into directory: ${XDP_TOOLS_DIR}."

DEBIAN_FRONTEND="noninteractive" apt-get install -y wget build-essential pkg-config python3 \
    libelf-dev clang llvm gcc-multilib "linux-headers-$(uname -r)" linux-tools-common "linux-tools-$(uname -r)" \
    iproute2 libpcap-dev iputils-ping tcpdump \

mkdir -p ${XDP_TOOLS_DIR}
cd ${XDP_TOOLS_DIR} || exit
wget https://github.com/xdp-project/xdp-tools/releases/download/v${XDP_TOOLS_VER}/xdp-tools-${XDP_TOOLS_VER}.tar.gz
tar -zxvf xdp-tools-${XDP_TOOLS_VER}.tar.gz --strip-components 1
rm ./xdp-tools-${XDP_TOOLS_VER}.tar.gz
./configure && make && make install && cd ./lib/libbpf/src && make install
