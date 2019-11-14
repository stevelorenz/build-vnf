#
# About: Docker container for functional tests
#

FROM bitnami/minideb:buster

ENV TESTS_SRC /opt/pktgen_tests
RUN install_packages wget ca-certificates iproute2 net-tools telnet tcpdump \
        python3 python3-pip python3-dev iputils-ping
RUN pip3 install setuptools wheel
RUN pip3 install scapy

RUN mkdir -p $TESTS_SRC

# Copy test scripts using Scapy

WORKDIR $TESTS_SRC

CMD ["bash"]
