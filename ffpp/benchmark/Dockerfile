#
# About: Docker container for latency benchmarking
#
# Build and installed tools:
#
# - Sockperf (https://github.com/mellanox/sockperf)
#

FROM bitnami/minideb:buster

ENV LB_SRC /opt/latency_benchmark
RUN install_packages wget ca-certificates iproute2 net-tools telnet tcpdump
RUN mkdir -p ${LB_SRC}

WORKDIR ${LB_SRC}
# MARK: Compile Sockperf from source code require much resources... Therefore, download deb from this link
RUN wget http://ftp.br.debian.org/debian/pool/main/s/sockperf/sockperf_3.6-2_amd64.deb -O ./sockperf.deb \
    && dpkg -i ./sockperf.deb \
    && rm ./sockperf.deb

CMD ["bash"]
