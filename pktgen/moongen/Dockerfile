FROM ubuntu:20.04

ENV MOONGEN_VERSION master_latest

RUN apt-get update --fix-missing \
    && DEBIAN_FRONTEND="noninteractive" apt-get install -y wget git ca-certificates iproute2 net-tools telnet tcpdump iputils-ping procps \
       build-essential cmake linux-headers-`uname -r` pciutils libnuma-dev \
       sudo python3 python3-setuptools zlib1g-dev kmod strace \
    && rm -rf /var/lib/apt/lists /var/cache/apt/archives

RUN mkdir -p /moongen
WORKDIR /moongen
RUN git clone --recurse-submodules https://github.com/emmericp/moongen /moongen

RUN mkdir -p /moongen/local
COPY ./CMakeLists.txt /moongen/libmoon/CMakeLists.txt
COPY ./dpdk-conf.lua /moongen/local/dpdk-conf.lua

RUN bash ./build.sh

CMD ["bash"]
