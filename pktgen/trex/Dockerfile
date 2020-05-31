FROM ubuntu:20.04

ENV TREX_VERSION v2.81

LABEL RUN docker run --privileged --cap-add=ALL -v /mnt/huge:/mnt/huge -v /lib/modules:/lib/modules:ro -v /sys/bus/pci/devices:/sys/bus/pci/devices -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev --name NAME -e NAME=NAME -e IMAGE=IMAGE IMAGE

RUN apt-get update --fix-missing \
    && apt-get install -y wget ca-certificates iproute2 net-tools telnet tcpdump iputils-ping procps \
       sudo gcc g++ python3 python3-setuptools zlib1g-dev pciutils kmod strace \
    && rm -rf /var/lib/apt/lists /var/cache/apt/archives

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

RUN mkdir -p /trex/${TREX_VERSION}/local
COPY ./run_basic_test.sh /trex/${TREX_VERSION}/local/run_basic_test.sh
COPY ./stf_path.py /trex/${TREX_VERSION}/local/stf_path.py
COPY ./run_stateless_test.py /trex/${TREX_VERSION}/local/run_stateless_test.py

CMD ["bash"]
