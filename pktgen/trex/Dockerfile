FROM ubuntu:18.04

ENV TREX_VER v2.74

LABEL RUN docker run --privileged --cap-add=ALL -v /mnt/huge:/mnt/huge -v /lib/modules:/lib/modules:ro -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev --name NAME -e NAME=NAME -e IMAGE=IMAGE IMAGE

RUN apt-get update --fix-missing \
    && apt-get install -y wget ca-certificates iproute2 net-tools telnet tcpdump iputils-ping procps \
       sudo gcc g++ python3 python3-setuptools zlib1g-dev pciutils kmod strace \
    && rm -rf /var/lib/apt/lists /var/cache/apt/archives

RUN mkdir /trex
WORKDIR /trex
RUN wget --no-cache https://trex-tgn.cisco.com/trex/release/${TREX_VER}.tar.gz && \
    tar -xzvf ./${TREX_VER}.tar.gz && \
    rm ./${TREX_VER}.tar.gz

COPY ./trex_cfg.yaml /etc/trex_cfg.yaml

WORKDIR /trex/${TREX_VER}

RUN ln -s /usr/bin/python3 /usr/bin/python && \
    tar -xzf trex_client_${TREX_VER}.tar.gz

CMD ["bash"]
