#
# Dockerfile ONLY used for the development and tests of FFPP library.
#
# - Installed dependencies:
#   - DPDK: v18.11 LTS
#

FROM ubuntu:18.04

ENV FFPP_DIR /ffpp
ARG DPDK_VERSION=v18.11

# Build DPDK

ENV RTE_SDK=/root/dpdk
ENV RTE_TARGET=x86_64-native-linuxapp-gcc
# Disable RDRAND, conflict with valgrind memory checker
ENV EXTRA_CFLAGS="-mno-rdrnd"

RUN apt-get update && apt-get install -y net-tools \
    build-essential automake python-pip libcap-ng-dev gawk pciutils kmod \
    linux-headers-$(uname -a | awk '{print $3}') \
    git hugepages clang doxygen \
    libnuma-dev libpcap-dev

WORKDIR /root

RUN git clone https://dpdk.org/git/dpdk "${RTE_SDK}" && \
    cd dpdk && \
    git checkout -b lts "${DPDK_VERSION}" && \
    make config T=${RTE_TARGET} && \
    make -j $(nproc) && make install && \
    ln -sf "${RTE_SDK}/build" "${RTE_SDK}/${RTE_TARGET}"


# Install utils/debug/perf tools for DPDK applications
RUN apt-get install -y gdb linux-tools-$(uname -r) lldb valgrind iputils-ping iproute2 tcpdump

# Build FFPP library
RUN mkdir -p ${FFPP_DIR}
COPY . ${FFPP_DIR}
WORKDIR ${FFPP_DIR}
RUN make clean && make lib -j $(nproc)

CMD ["bash"]
