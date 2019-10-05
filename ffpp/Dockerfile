#
# Dockerfile ONLY used for the development and tests of FFPP library.
#
# - Installed dependencies:
#   - DPDK: v19.08
#

FROM ubuntu:18.04

ARG DPDK_VERSION=v19.08

#
# Build DPDK from source
#
ENV RTE_SDK=/opt/dpdk
ENV RTE_TARGET=x86_64-native-linuxapp-gcc

# Disable RDRAND, conflict with valgrind memory checker
ENV EXTRA_CFLAGS="-g3 -Wno-error=maybe-uninitialized -fPIC -mno-rdrnd"

# Build essentials for DPDK
RUN apt-get update && apt-get install -y git build-essential \
    pciutils kmod linux-headers-$(uname -a | awk '{print $3}') hugepages \
    libnuma-dev libpcap-dev

# Test utilities
RUN apt-get install -y gdb valgrind iputils-ping iproute2 python3

RUN mkdir -p ${RTE_SDK}
WORKDIR ${RTE_SDK}
RUN git clone https://dpdk.org/git/dpdk "${RTE_SDK}" && \
    git checkout -b dev "${DPDK_VERSION}" && \
    make config T=${RTE_TARGET}

# Modify config file here

RUN make -j $(nproc) && \
    ln -sf "${RTE_SDK}/build" "${RTE_SDK}/${RTE_TARGET}"
#
# Build FFPP library
#
ENV FFPP_DIR /ffpp
RUN mkdir -p ${FFPP_DIR}
COPY . ${FFPP_DIR}
WORKDIR ${FFPP_DIR}
RUN make clean && make lib -j $(nproc)

CMD ["bash"]
