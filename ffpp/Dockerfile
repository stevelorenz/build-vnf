FROM ubuntu:20.04

#
# Install common build dependencies
#

RUN apt-get update && DEBIAN_FRONTEND="noninteractive" apt-get install -y \
	build-essential \
	clang \
	cmake \
	gcc-multilib \
	git \
	libelf-dev \
	libnuma-dev \
	libpcap-dev \
	linux-tools-common \
	llvm \
	m4 \
	meson \
	ninja-build\
	pkg-config \
	python3 \
	python3-pyelftools \
	wget

WORKDIR /opt/

#
# Build and install dependencies that cannot be installed directly by a package manager
# This is just a WORKAROUND due to issues such as package versions or compilation configurations...
#

# Build xdp-tools from source using git.
# The reason to use git instead of downloading a tarball or deb package is that the
# xdp-tools project is currently still under heavy development without a quick release of stable deb packages.
# xdp-tools is based on the libbpf (the forked one in xdp project).
# It's easy to use git submodule to fetch the exact version used by xdp-tools and make sure it compiles.
ENV XDP_TOOLS_VER="v1.2.0"
ENV XDP_TOOLS_DIR="/opt/xdp-tools"
RUN mkdir -p ${XDP_TOOLS_DIR} && \
	git clone https://github.com/xdp-project/xdp-tools && \
	cd ./xdp-tools && git checkout -b ${XDP_TOOLS_VER} ${XDP_TOOLS_VER} && \
	git submodule init && git submodule update && \
	./configure && make && make install && \
	cd ./lib/libbpf/src && make install

# Let the build system and linker to find the libbpf.so
ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib64/pkgconfig
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/lib64

# Build DPDK from source.
# The deb package (of Ubuntu 20.04) is not used currently since it does not
# contain AF_XDP PMD. Will use the deb package when AF_XDP is fully supported.
ENV DPDK_VER="21.11"
ENV RTE_SDK="/opt/dpdk"
RUN mkdir -p ${RTE_SDK} && \
	wget http://fast.dpdk.org/rel/dpdk-${DPDK_VER}.tar.xz && \
	tar -xJf dpdk-${DPDK_VER}.tar.xz -C ${RTE_SDK} --strip-components 1
COPY ./configs/dpdk/meson_options.txt ${RTE_SDK}/meson_options.txt
RUN cd ${RTE_SDK} && meson build && cd build && \
	ninja && ninja install && ldconfig && \
	rm -rf /opt/dpdk-${DPDK_VER}.tar.xz

#
# Install dependencies that be installed directly by a package manager
#

# TODO: Remove unused dependencies !!!
# TODO: Check if clang can be used to build pybind11 in e.g. Ubuntu 22.04
RUN debian_frontend="noninteractive" apt-get install -y \
	libbenchmark-dev \
	libboost-coroutine-dev \
	libboost-dev \
	libboost-program-options-dev \
	libboost-system-dev \
	libcpufreq-dev \
	libczmq-dev \
	libfmt-dev \
	libgoogle-glog-dev \
	libgrpc++-dev \
	libjansson-dev \
	libmnl-dev \
	libmsgsl-dev \
	libprotobuf-dev \
	libtins-dev \
	libyaml-cpp-dev \
	protobuf-compiler-grpc \
	python3-dev \
	python3-numpy \
	python3-pip \
	python3-zmq

# Development, testing and debugging tools.
# These packages are NOT needed in production.
RUN debian_frontend="noninteractive" apt-get install -y \
	clang-tidy \
	cppcheck \
	gcovr \
	google-mock \
	googletest \
	iproute2 \
	iputils-ping \
	# For Linux perf tool: CPU hardware couters and events.
	linux-tools-`uname -r`\
	linux-tools-common \
	net-tools \
	tcpdump \
	# Mainly for checking and profiling WITHOUT modification of the source code
	valgrind

# TODO: Check if miniconda is a better option.
# Latest pybind11 is needed for bytesarray and clang support
RUN pip3 install -q "docker==5.0.3" "pybind11[global]==2.8.1"

#
# Finally! Build and install FFPP targets with release version. :)
#

ENV FFPP_PATH /ffpp
RUN mkdir -p ${FFPP_PATH}
COPY . ${FFPP_PATH}
WORKDIR ${FFPP_PATH}
RUN meson build --buildtype=release && \
	cd build && \
	ninja && ninja install && ldconfig

ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/x86_64-linux-gnu/pkgconfig
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib/x86_64-linux-gnu
ENV PYTHONPATH="${PYTHONPATH}:/usr/local/lib/python3.8/site-packages"

WORKDIR ${FFPP_PATH}

CMD ["bash"]
