#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit
fi

if [[ -z ${XDP_TOOLS_DIR} ]]; then
    readonly XDP_TOOLS_DIR="/opt/xdp-tools"
fi

if [[ -d ${XDP_TOOLS_DIR} ]]; then
    echo "Error: XDP directory already exists."
    echo "Please remove the directory: ${XDP_TOOLS_DIR} and re-run the script for potential updates."
    exit 1
fi

TEST=false
XDP_TOOLS_VER="0.0.3"

set -o errexit
set -o nounset
set -o pipefail

function script_usage() {
    cat <<EOF
Usage:
     -h|--help                  Displays this help
     -t|--test                  Install the test version (pre-releases).
EOF
}

function parse_params() {
    local param

    while [[ $# -gt 0 ]]; do
        param="$1"
        shift
        case $param in
        -h | --help)
            script_usage
            exit 0
            ;;
        -t | --test)
            echo "- Run setup with test version."
            TEST=true
            XDP_TOOLS_VER="1.0.1"
            ;;
        *)
            echo "Error: Invalid parameter was provided: $param"
            script_usage
            ;;
        esac
    done
}

function cleanup() {
    rm -f /usr/lib64/libbpf.*
    rm -f /usr/lib64/pkgconfig/libbpf.pc
    rm -f /usr/local/lib/libxdp.*
    rm -f /usr/local/lib/pkgconfig/libxdp.pc
}

parse_params "$@"
cleanup

echo "* Download, build and install XDP-Tools (version ${XDP_TOOLS_VER})."
echo "- Dependency packages are also installed."
echo "- Source is downloaded into directory: ${XDP_TOOLS_DIR}."

DEBIAN_FRONTEND="noninteractive" apt-get install -y wget build-essential pkg-config python3 \
    libelf-dev clang llvm gcc-multilib "linux-headers-$(uname -r)" linux-tools-common "linux-tools-$(uname -r)" \
    iproute2 libpcap-dev iputils-ping tcpdump

mkdir -p ${XDP_TOOLS_DIR}
cd ${XDP_TOOLS_DIR} || exit

if [[ "$TEST" == true ]]; then
    # - For pre-releases
    git clone https://github.com/xdp-project/xdp-tools.git .
    git checkout tags/v${XDP_TOOLS_VER} -b v${XDP_TOOLS_VER}
    git submodule init && git submodule update
else
    # - For stable releases
    wget https://github.com/xdp-project/xdp-tools/releases/download/v${XDP_TOOLS_VER}/xdp-tools-${XDP_TOOLS_VER}.tar.gz
    tar -zxvf xdp-tools-${XDP_TOOLS_VER}.tar.gz --strip-components 1
    rm ./xdp-tools-${XDP_TOOLS_VER}.tar.gz
fi

./configure && make -j $(nproc) install && cd ./lib/libbpf/src && make -j $(nproc) install

if ! grep -Fq "/usr/lib64/pkgconfig" /etc/environment; then
    echo "- Setup ENV variables for pkg-config in /etc/environment ."
    echo "export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib64/pkgconfig" >>/etc/environment
    echo "export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/lib64" >>/etc/environment
fi

echo "- Add /usr/lib64 to ldconfig path for libbpf."
echo "/usr/lib64" >/etc/ld.so.conf.d/libbpf.conf
ldconfig

echo "- Current libs version:"
echo "  - libbpf: $(pkg-config --modversion libbpf)"
if [[ $TEST == true ]]; then
    echo "  - libxdp: $(pkg-config --modversion libxdp)"
fi
