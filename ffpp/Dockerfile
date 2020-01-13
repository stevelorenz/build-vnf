#
# Dockerfile ONLY used for the development and tests of FFPP library.
#
# This image includes also test and debug utilities and has a relative big size. (about 1.0GB)
# Most DPDK PMDs and features are disabled to reduce image size.
# It is not designed/suitable for deployment usage.
#

#
# Build DPDK from source.
#
FROM ubuntu:18.04 as builder

ENV DPDK_VER=19.08
ENV RTE_SDK=/opt/dpdk
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
    tar -xf dpdk-${DPDK_VER}.tar.xz -C ./dpdk --strip-components 1

WORKDIR ${RTE_SDK}

RUN make config T=${RTE_TARGET}
# Use customized compile-time DPDK configuration
# The goal here is to reduce the image size
COPY config ${RTE_SDK}/build/.config

RUN make -j $(nproc) EXTRA_CFLAGS="${EXTRA_CFLAGS}" && \
    make install && \
    ln -sf "${RTE_SDK}/build" "${RTE_SDK}/${RTE_TARGET}"

#
# Build FFPP with compiled DPDK.
#
FROM ubuntu:18.04 as ffpp

ENV DPDK_VER=19.08
ENV RTE_SDK=/opt/dpdk
ENV RTE_TARGET=x86_64-native-linuxapp-gcc
ENV EXTRA_CFLAGS="-g3 -Wno-error=maybe-uninitialized -fPIC -mno-rdrnd"

RUN mkdir -p ${RTE_SDK}
RUN apt-get update && apt-get install -y build-essential pciutils hugepages libnuma-dev libpcap-dev xz-utils \
    liblua5.3-dev lua5.3 \
    # Debug and test utilities.
    gdb valgrind iputils-ping iproute2 python3 tcpdump net-tools && \
    rm -rf /var/lib/apt/lists /var/cache/apt/archives

COPY --from=builder /usr/local/bin /usr/local/bin
COPY --from=builder /usr/local/include /usr/local/include
COPY --from=builder /usr/local/lib /usr/local/lib/dpdk
COPY --from=builder /usr/local/sbin /usr/local/sbin
COPY --from=builder /usr/local/share/dpdk /usr/local/share/dpdk

COPY --from=builder ${RTE_SDK}/mk ${RTE_SDK}/mk
COPY --from=builder ${RTE_SDK}/buildtools ${RTE_SDK}/buildtools

RUN mkdir -p ${RTE_SDK}/build \
  && ln -s /usr/local/lib/dpdk ${RTE_SDK}/build/lib \
  && ln -s /usr/local/include/dpdk ${RTE_SDK}/build/include \
  && ln -sf "${RTE_SDK}/build" "${RTE_SDK}/${RTE_TARGET}"

COPY config ${RTE_SDK}/build/.config

# Build FFPP library
ENV FFPP_DIR /ffpp

RUN mkdir -p ${FFPP_DIR}
COPY . ${FFPP_DIR}
WORKDIR ${FFPP_DIR}
RUN make clean && make lib

CMD ["bash"]
