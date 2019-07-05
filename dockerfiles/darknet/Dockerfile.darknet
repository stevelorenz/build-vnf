#
# Dockerfile for DarkNet (NN in C)
# WARN: This dockerfile is used for dev and debugging. Some files are copied into container based on the build-vnf directory structure
#
# - For CPU ONLY (without CUDA related stuffs)
# - Use the fork of AlexyAB for AVX acceleration
#

FROM ubuntu:18.04

# Install Python3

RUN apt-get update && \
    apt-get install -y python3-pip python3-dev && \
    cd /usr/local/bin && \
    ln -s /usr/bin/python3 python && \
    pip3 install --upgrade pip

# Build darknet

WORKDIR /root

RUN apt-get update && apt-get install -y \
    autoconf automake libtool build-essential git \
    wget

RUN git clone https://github.com/AlexeyAB/darknet && \
    # git clone https://github.com/pjreddie/darknet && \
    cd darknet && \
    # Build darknet library
    sed -i '0,/LIBSO=0/s//LIBSO=1/' Makefile && \
    # Enable AVX acceleration
    sed -i '0,/AVX=0/s//AVX=1/' Makefile && \
    # Enable multi-CPU acceleration
    # sed -i '0,/OPENMP=0/s//OPENMP=1/' Makefile \
    make -j $(nproc)

RUN cp /root/darknet/libdarknet.so /usr/lib

WORKDIR /root/darknet

RUN wget https://pjreddie.com/media/files/yolov2.weights && \
    wget https://pjreddie.com/media/files/yolov2-tiny-voc.weights

CMD ["bash"]
