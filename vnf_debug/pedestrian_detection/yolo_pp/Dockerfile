FROM ubuntu:xenial
MAINTAINER manuel.peuster@uni-paderborn.de

RUN apt-get update && apt-get install -y \
    net-tools \
    iputils-ping \
    iproute \
    software-properties-common \
    sudo \
    git \
    python3-pip \
    wget \
    unzip

RUN pip3 install --upgrade setuptools && \
    pip3 install Cython numpy==1.15

RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo

WORKDIR /app
RUN git clone https://github.com/zrbzrb1106/yolov2.git


RUN git clone https://github.com/thtrieu/darkflow.git && \
    cd darkflow && \
    python3 setup.py build_ext --inplace && \
    pip3 install -e .
RUN wget https://pjreddie.com/media/files/yolov2.weights -O darkflow/yolo.weights

WORKDIR /app/yolov2/scripts
RUN chmod -R 755 ./*
RUN sh ./setup_env.sh

WORKDIR /app/darkflow
RUN sudo flow --model cfg/yolo.cfg --load yolo.weights --savepb; exit 0
RUN cd /app/yolov2 && mkdir model
RUN cp built_graph/* /app/yolov2/model

WORKDIR /app/yolov2
RUN git clone https://github.com/philferriere/cocoapi.git && \
    wget http://images.cocodataset.org/zips/val2017.zip && \
    wget http://images.cocodataset.org/annotations/annotations_trainval2017.zip && \
    mkdir ./cocoapi/images && \
    unzip ./val2017.zip -d ./cocoapi/images/ && \
    unzip ./annotations_trainval2017.zip -d ./cocoapi/
RUN cd cocoapi/PythonAPI && python3 setup.py build_ext --inplace

# Switch to dev branch, currently new commits are added into dev branch by zrbzrb1106
WORKDIR /app/yolov2
RUN git fetch origin dev && \
    git checkout -b dev origin/dev

# Add python libs from zrbzrb1106/yolov2 into PYTHON python
ENV PYTHONPATH "${PYTHONPATH}:/app/yolov2"
# TODO: Remove dependencies for these libs
RUN pip3 install scipy sklearn

# Use python3 as the default python
RUN rm -rf /usr/bin/python && ln -s /usr/bin/python3 /usr/bin/python

# Build DPDK

ENV RTE_SDK=/root/dpdk
ENV RTE_TARGET=x86_64-native-linuxapp-gcc

RUN apt-get update && apt-get install -y net-tools \
        build-essential automake python-pip libcap-ng-dev gawk pciutils kmod \
        linux-headers-$(uname -a | awk '{print $3}') \
        git hugepages clang doxygen \
        libnuma-dev libpcap-dev

WORKDIR /root

RUN \
        git clone http://dpdk.org/git/dpdk "${RTE_SDK}" && \
        cd dpdk && \
        git checkout -b lts v18.11 && \
        make config T=${RTE_TARGET} && \
        make && \
        ln -sf "${RTE_SDK}/build" "${RTE_SDK}/${RTE_TARGET}"

# Add fastio_user library
WORKDIR /app
RUN git clone https://github.com/stevelorenz/build-vnf.git

WORKDIR /app/build-vnf
RUN git fetch origin dev-zuo && \
    git checkout -b dev-zuo origin/dev-zuo && \
    cp -r ./fastio_user /fastio_user

WORKDIR /fastio_user
RUN make

RUN apt-get install -y gdb
RUN pip3 install ipdb

CMD /bin/bash
